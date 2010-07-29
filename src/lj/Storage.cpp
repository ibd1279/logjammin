/*!
 \file Storage.cpp
 \brief LJ Storage implementation.
 \author Jason Watson
 
 Copyright (c) 2010, Jason Watson
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 * Neither the name of the LogJammin nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "lj/Storage.h"

#include "build/default/config.h"
#include "lj/All_record_set.h"
#include "lj/Exception.h"
#include "lj/Logger.h"
#include "lj/Standard_record_set.h"

#include <sstream>
#include <sys/time.h>

// XXX This should be moved somewhere for portability.
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

using tokyo::TextSearcher;
using tokyo::TagSearcher;

// ====================================================================
// Opening a storage object.
// ====================================================================
namespace
{
    inline std::string root_data_directory(const lj::Bson& server_config)
    {
        return lj::bson_as_string(server_config.nav("data_directory"));
    }
    
    inline std::string storage_config_path(const lj::Bson& server_config,
                                           const std::string& name)
    {
        return root_data_directory(server_config) + "/" + name + "/config";
    }
    
    inline std::string storage_database_path(const lj::Bson& server_config,
                                             const std::string& name,
                                             const lj::Bson& config,
                                             const std::string& config_key)
    {
        const std::string& r = root_data_directory(server_config);
        const std::string& p = lj::bson_as_string(config.nav(config_key));
        assert(p.size() > 0);
        return (p.at(0) == '/') ? p : (r + "/" + name + "/" + p);
    }
                                             
    
    void tune_hash_index(TCHDB* db, const void* ptr)
    {
        //const Bson *bn = static_cast<const Bson *>(ptr);
        tchdbtune(db, 514229, 8, 11, HDBTLARGE | HDBTBZIP);
        
        // XXX config other things like compression type.
    }
    
    void tune_tree_index(TCBDB* db, const void* ptr)
    {
        const lj::Bson* bn = static_cast<const lj::Bson*>(ptr);
        if (lj::bson_as_string(bn->nav("compare")).compare("lex") == 0)
        {
            tcbdbsetcmpfunc(db, tcbdbcmplexical, NULL);
            lj::Log::info.log("  Using lexical for compares") << lj::Log::end;
        }
        else if (lj::bson_as_string(bn->nav("compare")).compare("int32") == 0)
        {
            tcbdbsetcmpfunc(db, tcbdbcmpint32, NULL);
            lj::Log::info.log("  Using int32 for compares") << lj::Log::end;
        }
        else
        {
            tcbdbsetcmpfunc(db, tcbdbcmpint64, NULL);
            lj::Log::info.log("  Using int64 for compares") << lj::Log::end;
        }
        tcbdbtune(db, 256, 512, 65498, 9, 11, BDBTLARGE | BDBTBZIP);
    }
    
    void tune_text_index(TCQDB* db, const void* ptr)
    {
        //const Bson *bn = static_cast<const Bson *>(ptr);
        tcqdbtune(db, 10000000, QDBTLARGE | QDBTBZIP);
        
        // XXX config other things like compression type.
    }
    
    // XXX Add configuration method for tags
    void tune_word_index(TCWDB* db, const void* ptr)
    {
        //const Bson *bn = static_cast<const Bson *>(ptr);
        tcwdbtune(db, 10000000, WDBTLARGE | WDBTBZIP);
        
        // XXX config other things like compression type.
    }
    
    void tune_fixed_index(TCFDB* db, const void* ptr)
    {
        tcfdbtune(db, sizeof(bool), -1);
    }
    
    template<typename T>
    void open_indices(const lj::Bson& server_config,
                      const std::string& name,
                      const lj::Bson& indices,
                      int open_flags,
                      typename T::Tune_function_pointer tune_function,
                      std::map<std::string, T*>& dest)
    {
        const std::string& dir = root_data_directory(server_config);
        
        lj::Log::info.log("Opening %s indices under [%s].") << T::k_db_type << dir << lj::Log::end;
        for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = indices.to_map().begin();
             indices.to_map().end() != iter;
             ++iter)
        {
            const std::string& index_name = (*iter).first;
            lj::Bson* cfg = (*iter).second;
            if (!cfg->exists())
            {
                lj::Log::debug.log("  Unable to open index [%s] because it has been deleted.") << index_name << lj::Log::end; 
                continue;
            }
            
            if (!cfg->nav("file").exists() ||
                !cfg->nav("field").exists())
            {
                lj::Log::error.log("Unable to open index [%s] because file or field is not set.") << index_name << lj::Log::end; 
                continue;
            }
            
            const std::string& indexfile = storage_database_path(server_config,
                                                                 name,
                                                                 *cfg,
                                                                 "file");
            const std::string& field(lj::bson_as_string(cfg->nav("field")));
            
            lj::Log::debug.log("  Opening [%s] for [%s]") << indexfile << field << lj::Log::end;
            T* db = new T(indexfile,
                          open_flags,
                          tune_function,
                          cfg);
            dest.insert(std::pair<std::string, T*>(field, db));
        }
    }
    
    template<typename T>
    void close_indices(const lj::Bson& server_config,
                       const std::string& name,
                       const std::map<std::string, T*>& indices)
    {
        const std::string& dir = root_data_directory(server_config);
        if (indices.size())
        {
            lj::Log::info.log("Closing %s indicies under [%s/%s].") << T::k_db_type << dir << name << lj::Log::end;
            typedef typename std::map<std::string, T*>::const_iterator Index_map_iter;
            for (Index_map_iter iter = indices.begin();
                 indices.end() != iter;
                 ++iter)
            {
                lj::Log::info.log("Closing %s index for field [%s].") << T::k_db_type << iter->first << lj::Log::end;
                delete iter->second;
            }
        }
    }
}; // namespace

namespace lj
{
    Storage::Storage(const std::string &name, const lj::Bson& server_config) : db_(NULL), fields_tree_(), fields_hash_(), fields_text_(), fields_tag_(), config_(NULL), server_config_(server_config), name_(name)
    {
        const std::string& dir = root_data_directory(server_config_);
        const std::string& configfile = storage_config_path(server_config_, name_);
        
        Log::info.log("Loading configuration from [%s].") << configfile << Log::end;
        config_ = bson_load(configfile);
        Log::debug.log("Loaded Settings [%s].") << bson_as_pretty_string(*config_) << Log::end;
        
        const std::string& dbfile = storage_database_path(server_config_,
                                                          name_,
                                                          *config_,
                                                          "main/file");
        Log::info.log("Opening database [%s].") << dbfile << Log::end;
        db_ = new tokyo::Tree_db(dbfile,
                                 BDBOREADER | BDBOWRITER | BDBOCREAT | BDBOLCKNB,
                                 &tune_tree_index,
                                 config_->path("main"));
        
        const std::string& journalfile = storage_database_path(server_config_,
                                                               name_,
                                                               *config_,
                                                               "journal/file");
        Log::info.log("Opening journal [%s].") << journalfile << Log::end;
        journal_ = new tokyo::Fixed_db(journalfile,
                                       FDBOREADER | FDBOWRITER | FDBOCREAT | FDBOLCKNB,
                                       &tune_fixed_index,
                                       config_->path("journal"));
        
        open_indices<tokyo::Hash_db>(server_config_,
                                     name_,
                                     config_->nav("index/hash"),
                                     HDBOREADER | HDBOWRITER | HDBOCREAT | HDBOLCKNB,
                                     &tune_hash_index,
                                     fields_hash_);
        
        open_indices<tokyo::Tree_db>(server_config_,
                                     name_,
                                     config_->nav("index/tree"),
                                     BDBOREADER | BDBOWRITER | BDBOCREAT | BDBOLCKNB,
                                     &tune_tree_index,
                                     fields_tree_);
        
        open_indices<TextSearcher>(server_config_,
                                   name,
                                   config_->nav("index/text"),
                                   QDBOREADER | QDBOWRITER | QDBOCREAT | QDBOLCKNB,
                                   &tune_text_index,
                                   fields_text_);
        
        open_indices<TagSearcher>(server_config_,
                                  name,
                                  config_->nav("index/tag"),
                                  WDBOREADER | WDBOWRITER | WDBOCREAT | WDBOLCKNB,
                                  &tune_word_index,
                                  fields_tag_);
        
        Log::info.log("Checkpointing [%s] after startup.") << dir << Log::end;
        checkpoint();
    }
    
    Storage::~Storage()
    {
        const std::string& dir = root_data_directory(server_config_);
        
        Log::info.log("Checkpointing [%s/%s] before shutdown.") << dir << name_ << Log::end;
        checkpoint();
        
        close_indices<TagSearcher>(server_config_, name_, fields_tag_);
        close_indices<TextSearcher>(server_config_, name_, fields_text_);
        close_indices<tokyo::Tree_db>(server_config_, name_, fields_tree_);
        close_indices<tokyo::Hash_db>(server_config_, name_, fields_hash_);
        
        Log::info.log("Closing journal for [%s/%s].") << dir << name_ << Log::end;
        if (journal_)
        {
            delete journal_;
            journal_ = 0;
        }
        
        Log::info.log("Closing database for [%s/%s].") << dir << name_ << Log::end;
        if (db_)
        {
            delete db_;
            db_ = 0;
        }
        
        if (config_)
        {
            delete config_;
            config_ = 0;
        }
    }
}; // namespace lj

// ====================================================================
// indexing/deindexing a record in a storage object.
// ====================================================================
namespace
{
    std::pair<int, int> bson_to_storage_delta(const lj::Bson& ptr)
    {
        if(lj::bson_type_is_quotable(ptr.type()))
        {
            return std::pair<int, int>(4,5);
        }
        else
        {
            return std::pair<int, int>(0,0);
        }
    }
    
    template<typename T>
    T* get_index_by_field(const std::map<std::string, T*>& m,
                          const std::string& index)
    {
        typedef typename std::map<std::string, T*>::const_iterator Index_map_iter;
        Index_map_iter iter = m.find(index);
        if (m.end() == iter)
        {
            return NULL;
        }
        else
        {
            return (*iter).second;
        }
    }
    
    void hash_deindex(tokyo::Hash_db& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        char* bson = n.to_binary();
        std::pair<int, int> delta(bson_to_storage_delta(n));
        db.remove(bson + delta.first,
                  n.size() - delta.second);
        delete[] bson;
    }
    
    void hash_reindex(tokyo::Hash_db& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        char* bson = n.to_binary();
        std::pair<int, int> delta(bson_to_storage_delta(n));
        db.place(bson + delta.first,
                 n.size() - delta.second,
                 &key,
                 sizeof(unsigned long long));
        delete[] bson;
    }
    
    void tree_deindex(tokyo::Tree_db& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        char* bson = n.to_binary();
        std::pair<int, int> delta(bson_to_storage_delta(n));
        db.remove_from_existing(bson + delta.first,
                                n.size() - delta.second,
                                &key,
                                sizeof(unsigned long long));
        delete[] bson;
    }
    
    void tree_reindex(tokyo::Tree_db& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        char* bson = n.to_binary();
        std::pair<int, int> delta(bson_to_storage_delta(n));
        db.remove_from_existing(bson + delta.first,
                                n.size() - delta.second,
                                &key,
                                sizeof(unsigned long long));
        delete[] bson;
    }
    
    void text_deindex(tokyo::TextSearcher& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        db.remove(key, lj::bson_as_string(n));
    }
    
    void text_reindex(tokyo::TextSearcher& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        db.index(key, lj::bson_as_string(n));
    }
    
    void word_deindex(tokyo::TagSearcher& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        db.remove(key, lj::bson_as_value_string_set(n));
    }
    
    void word_reindex(tokyo::TagSearcher& db,
                      const lj::Bson& n,
                      const unsigned long long key)
    {
        db.index(key, lj::bson_as_value_string_set(n));
    }
    
    template<typename T>
    void perform_on_index(const std::string field_name,
                          T& index,
                          const bool allow_subfields,
                          const std::set<std::string>& subfields,
                          const lj::Bson& record,
                          const unsigned long long key,
                          void (*func)(T& db,
                                       const lj::Bson& n,
                                       const unsigned long long key))
    {
        const lj::Bson* n = record.path(field_name);
        if (!n)
        {
            return;
        }
        if (!n->exists())
        {
            return;
        }
        
        if (lj::bson_type_is_nested(n->type()) && 
            subfields.end() != subfields.find(field_name) &&
            allow_subfields)
        {
            lj::Log::debug.log("  index [%d] from [%s] nested %s index.") << key << field_name << T::k_db_type << lj::Log::end;
            for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = n->to_map().begin();
                 n->to_map().end() != iter;
                 ++iter)
            {
                func(index, *iter->second, key);
            }
        }
        else
        {
            lj::Log::debug.log("  index [%d] from [%s] %s index.") << key << field_name << T::k_db_type << lj::Log::end;
            func(index, *n, key);
        }
    }
    
    template<typename T>
    void perform_on_indices(const std::map<std::string, T*>& m,
                            const bool allow_subfields,
                            const std::set<std::string>& subfields,
                            const lj::Bson& record,
                            const unsigned long long key,
                            void (*func)(T& db,
                                         const lj::Bson& n,
                                         const unsigned long long key))
    {
        typedef typename std::map<std::string, T*>::const_iterator Index_map_iter;
        
        // Loop over all the indicies of this type.
        for (Index_map_iter iter = m.begin();
             m.end() != iter;
             ++iter)
        {
            perform_on_index<T>(iter->first,
                                *(iter->second),
                                allow_subfields,
                                subfields,
                                record,
                                key,
                                func);
        }
    }
}; // namespace

namespace lj
{
    Storage &Storage::deindex(const lj::Bson& record)
    {
        const lj::Bson* key_node = record.path("__key");
        if (!key_node)
        {
            return *this;
        }
        
        unsigned long long key = bson_as_int64(*key_node);
        if (!key)
        {
            return *this;
        }
        
        Log::debug.log("Deindex [%d].") << key << Log::end;
        perform_on_indices<tokyo::Tree_db>(fields_tree_,
                                           true,
                                           nested_indexing_,
                                           record,
                                           key,
                                           &tree_deindex);
        
        perform_on_indices<tokyo::Hash_db>(fields_hash_,
                                           true,
                                           nested_indexing_,
                                           record,
                                           key,
                                           &hash_deindex);
        
        perform_on_indices<tokyo::TextSearcher>(fields_text_,
                                                false,
                                                nested_indexing_,
                                                record,
                                                key,
                                                &text_deindex);
        perform_on_indices<tokyo::TagSearcher>(fields_tag_,
                                               false,
                                               nested_indexing_,
                                               record,
                                               key,
                                               &word_deindex);
        return *this;
    }
    
    Storage &Storage::reindex(const lj::Bson& record)
    {
        const lj::Bson* key_node = record.path("__key");
        if (!key_node)
        {
            return *this;
        }
        
        unsigned long long key = bson_as_int64(*key_node);
        if (!key)
        {
            return *this;
        }
        
        Log::debug.log("Reindex [%d].") << key << Log::end;
        perform_on_indices<tokyo::Tree_db>(fields_tree_,
                                           true,
                                           nested_indexing_,
                                           record,
                                           key,
                                           &tree_reindex);
        
        perform_on_indices<tokyo::Hash_db>(fields_hash_,
                                           true,
                                           nested_indexing_,
                                           record,
                                           key,
                                           &hash_reindex);
        
        perform_on_indices<tokyo::TextSearcher>(fields_text_,
                                                false,
                                                nested_indexing_,
                                                record,
                                                key,
                                                &text_reindex);
        perform_on_indices<tokyo::TagSearcher>(fields_tag_,
                                               false,
                                               nested_indexing_,
                                               record,
                                               key,
                                               &word_reindex);
        return *this;
    }
    
    void Storage::rebuild_field_index(const std::string& index)
    {
        tokyo::Tree_db* tree_ptr = get_index_by_field<tokyo::Tree_db>(fields_tree_, index);
        if (tree_ptr)
        {
            tree_ptr->truncate();
        }
        
        tokyo::Hash_db* hash_ptr = get_index_by_field<tokyo::Hash_db>(fields_hash_, index);
        if (hash_ptr)
        {
            hash_ptr->truncate();
        }
        
        tokyo::TextSearcher* text_ptr = get_index_by_field<tokyo::TextSearcher>(fields_text_, index);
        if (text_ptr)
        {
            text_ptr->truncate();
        }
        
        tokyo::TagSearcher* word_ptr = get_index_by_field<tokyo::TagSearcher>(fields_tag_, index);
        if (word_ptr)
        {
            word_ptr->truncate();
        }
        
        const std::string& dir = root_data_directory(server_config_);
        Log::info.log("Rebuilding [%s] indices in [%s/%s]") << index << dir << name_ << Log::end;
        tokyo::Tree_db::Enumerator* e = db_->forward_enumerator();
        lj::Bson record;
        while (e->more())
        {
            tokyo::DB::value_t k = e->next_key();
            tokyo::DB::value_t v = e->next();
            record.set_value(lj::Bson::k_document,
                             static_cast<char*>(v.first));
            unsigned long long key = *static_cast<unsigned long long*>(k.first);
            free(k.first);
            free(v.first);
            perform_on_index<tokyo::Tree_db>(index,
                                             *tree_ptr,
                                             true,
                                             nested_indexing_,
                                             record,
                                             key,
                                             &tree_reindex);
            perform_on_index<tokyo::Hash_db>(index,
                                             *hash_ptr,
                                             true,
                                             nested_indexing_,
                                             record,
                                             key,
                                             &hash_reindex);
            perform_on_index<tokyo::TextSearcher>(index,
                                                  *text_ptr,
                                                  false,
                                                  nested_indexing_,
                                                  record,
                                                  key,
                                                  &text_reindex);
            perform_on_index<tokyo::TagSearcher>(index,
                                                 *word_ptr,
                                                 false,
                                                 nested_indexing_,
                                                 record,
                                                 key,
                                                 &word_reindex);
        }
    }
}; // namespace lj

// ====================================================================
// Utility functions for a storage object.
// ====================================================================
namespace
{
    template<typename T>
    void truncate_index(const std::string& field_name, T* index)
    {
        lj::Log::info.log("  Truncating %s index for field [%s].") << T::k_db_type << field_name << lj::Log::end;
        index->truncate();
    }
    
    template<typename T>
    void truncate_indices(const std::map<std::string, T*>& m)
    {
        typedef typename std::map<std::string, T*>::const_iterator Index_map_iter;
        if (m.size())
        {
            lj::Log::info.log("Truncating %s indices.") << T::k_db_type << lj::Log::end;
            for (Index_map_iter iter = m.begin();
                 m.end() != iter;
                 ++iter)
            {
                truncate_index<T>((*iter).first,
                                 (*iter).second);
            }
        }
    }
    
    template<typename T>
    void optimize_index(const std::string& field_name, T* index)
    {
        lj::Log::info.log("  Optimizing %s index for field [%s].") << T::k_db_type << field_name << lj::Log::end;
        index->optimize();
    }
    
    template<typename T>
    void optimize_indices(const std::map<std::string, T*>& m)
    {
        typedef typename std::map<std::string, T*>::const_iterator Index_map_iter;
        if (m.size())
        {
            lj::Log::info.log("Optimizing %s indices.") << T::k_db_type << lj::Log::end;
            for (Index_map_iter iter = m.begin();
                 m.end() != iter;
                 ++iter)
            {
                optimize_index<T>((*iter).first,
                                  (*iter).second);
            }
        }
    }
}; // namespace

namespace lj
{
    void Storage::checkpoint()
    {
        tokyo::Fixed_db::Enumerator* e = journal_->enumerator();
        bool modified = false;
        
        while (e->more())
        {
            uint64_t key = e->next_key();
            tokyo::DB::value_t v = e->next();
            bool val = *static_cast<bool*>(v.first);
            free(v.first);
            
            modified = true;
            
            if (!val)
            {
                Log::info.log("  Repairing [%d].") << key << Log::end;
                lj::Bson record;
                at(key)->first(record);
                deindex(record);
                reindex(record);
            }
        }
        
        Log::info.log("  Clearing journal.") << Log::end;
        journal_->truncate();
        
        if (modified)
        {
            Log::info.log("  Backing up datafile.") << Log::end;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            std::ostringstream target;
            target << storage_database_path(server_config_,
                                            name_,
                                            *config_,
                                            "main/file");
            target << ".backup" << "." << (tv.tv_sec);
            db_->copy(target.str());
        }
    }
    
    void Storage::rebuild()
    {
        const std::string dir = root_data_directory(server_config_);
        Log::info.log("Truncating all indicies in [%s/%s]") << dir << name_ << Log::end;
        truncate_indices<tokyo::Tree_db>(fields_tree_);
        truncate_indices<tokyo::Hash_db>(fields_hash_);
        truncate_indices<tokyo::TextSearcher>(fields_text_);
        truncate_indices<tokyo::TagSearcher>(fields_tag_);
        
        Log::info.log("Rebuilding all indicies in [%s/%s]") << dir << name_ << Log::end;
        tokyo::Tree_db::Enumerator* e = db_->forward_enumerator();
        lj::Bson record;
        while (e->more())
        {
            tokyo::DB::value_t v = e->next();
            record.set_value(lj::Bson::k_document,
                             static_cast<char*>(v.first));
            free(v.first);
            reindex(record);
        }
    }
    
    void Storage::optimize()
    {
        optimize_indices<tokyo::TagSearcher>(fields_tag_);
        optimize_indices<tokyo::TextSearcher>(fields_text_);
        optimize_indices<tokyo::Hash_db>(fields_hash_);
        optimize_indices<tokyo::Tree_db>(fields_tree_);
    }
    
    std::auto_ptr<Record_set> Storage::at(const unsigned long long key) const
    {
        std::auto_ptr<Record_set> ptr = none();
        return ptr->include_key(key);
    }
    
    std::auto_ptr<Record_set> Storage::all() const
    {
        return std::auto_ptr<Record_set>(new All_record_set(this,
                                                            Record_set::k_intersection));
    }
    
    std::auto_ptr<Record_set> Storage::none() const
    {
        Standard_record_set* ptr = new Standard_record_set(this,
                                                           new std::set<unsigned long long>(),
                                                           Record_set::k_union);
        ptr->set_raw_size(0);
        return std::auto_ptr<Record_set>(ptr);
    }
    
    Storage &Storage::place(Bson &value)
    {
        unsigned long long key = lj::bson_as_int64(value.nav("__key"));
        unsigned long long original_key = key;
        
        try
        {
            begin_transaction();
            Log::debug.log("Placing [%llu]") << key << Log::end;
            
            if (key)
            {
                Log::debug.log("Deindexing previous record to clean house.") << Log::end;
                journal_start(key);
                deindex(value);
            }
            else
            {
                Log::debug.log("New record. calculating key.") << Log::end;
                unsigned long long *ptr = static_cast<unsigned long long *>(db_->max_key().first);
                key = (*ptr) + 1;
                free(ptr);
                Log::debug.log("New key is [%d].") << key << Log::end;
                journal_start(key);
            }
            
            Log::debug.log("Unique constraint check.") << Log::end;
            for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
                 fields_hash_.end() != iter;
                 ++iter)
            {
                Bson n(value.nav(iter->first));
                if (n.exists())
                {
                    check_unique(n, iter->first, iter->second);
                }
            }
            
            Log::debug.log("Place in DB.") << Log::end;
            value.nav("__key").set_value(Bson::k_int64, reinterpret_cast<char*>(&key));
            char* bson = value.to_binary();
            db_->place(&key,
                       sizeof(unsigned long long),
                       bson,
                       value.size());
            delete[] bson;
            reindex(value);
            commit_transaction();
            journal_end(key);
        }
        catch(Exception* ex)
        {
            deindex(value);
            value.nav("__key").set_value(Bson::k_int64, reinterpret_cast<char *>(&original_key));
            abort_transaction();
            reindex(value);
            journal_end(key);
            throw ex;
        }
        return *this;
    }
    
    Storage &Storage::remove(Bson &value)
    {
        unsigned long long key = bson_as_int64(value.nav("__key"));
        
        Log::debug.log("Removing [%llu] [%s]") << key << bson_as_pretty_string(value) << Log::end;
        
        if (key)
        {
            try
            {
                journal_start(key);
                begin_transaction();
                deindex(value);
                db_->remove(&key, sizeof(unsigned long long));
                commit_transaction();
                journal_end(key);
                value.nav("__key").destroy();
            }
            catch (Exception* ex)
            {
                abort_transaction();
                deindex(value);
                reindex(value);
                journal_end(key);
                throw ex;
            }
        }
        return *this;
    }
    
    Storage &Storage::check_unique(const Bson& n, const std::string& name, tokyo::DB* index)
    {
        if (lj::bson_type_is_nested(n.type()) &&
            nested_indexing_.end() != nested_indexing_.find(name))
        {
            Log::debug.log("checking children of [%s].") << name << Log::end;
            
            for (Linked_map<std::string, Bson*>::const_iterator iter = n.to_map().begin();
                 n.to_map().end() != iter;
                 ++iter)
            {
                char* bson = iter->second->to_binary();
                std::pair<int, int> delta(bson_to_storage_delta(*(iter->second)));
                tokyo::DB::value_t existing = index->at(bson + delta.first,
                                                        iter->second->size() - delta.second);
                delete[] bson;
                if (existing.first)
                {
                    throw new Exception("StorageError",
                                        std::string("Unable to place record because of unique constraint [").append(name).append("]."));
                }
            }
        }
        else
        {
            Log::debug.log("Checking value of [%s].") << name << Log::end;
            char *bson = n.to_binary();
            std::pair<int, int> delta(bson_to_storage_delta(n));
            tokyo::DB::value_t existing = index->at(bson + delta.first,
                                                    n.size() - delta.second);
            delete[] bson;
            if (existing.first)
            {
                throw new Exception("StorageError",
                                    std::string("Unable to place record because of unique constraint [").append(name).append("]."));
            }
        }
        return *this;
    }
    
    void Storage::journal_start(const unsigned long long key)
    {
        Log::debug.log("Starting journaling for [%d]") << key << Log::end;
        
        bool complete = false;
        
        journal_->start_writes();
        journal_->place(&key,
                        sizeof(unsigned long long),
                        &complete,
                        sizeof(bool));
        journal_->save_writes();
    }
    
    void Storage::journal_end(const unsigned long long key)
    {
        Log::debug.log("Ending journaling for [%d]") << key << Log::end;
        
        bool complete = true;
        
        journal_->start_writes();
        journal_->place(&key,
                        sizeof(unsigned long long),
                        &complete,
                        sizeof(bool));
        journal_->save_writes();
    }
    
    void Storage::begin_transaction()
    {
        for (std::map<std::string, tokyo::Tree_db*>::const_iterator iter = fields_tree_.begin();
             fields_tree_.end() != iter;
             ++iter)
        {
            iter->second->start_writes();
        }
        for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            iter->second->start_writes();
        }
        db_->start_writes();
    }
    
    void Storage::commit_transaction()
    {
        db_->save_writes();
        for (std::map<std::string, tokyo::Hash_db*>::reverse_iterator iter = fields_hash_.rbegin();
             fields_hash_.rend() != iter;
             ++iter)
        {
            iter->second->save_writes();
        }
        for (std::map<std::string, tokyo::Tree_db *>::reverse_iterator iter = fields_tree_.rbegin();
             fields_tree_.rend() != iter;
             ++iter)
        {
            iter->second->save_writes();
        }
    }
    
    void Storage::abort_transaction()
    {
        db_->abort_writes();
        for (std::map<std::string, tokyo::Hash_db*>::reverse_iterator iter = fields_hash_.rbegin();
             fields_hash_.rend() != iter;
             ++iter)
        {
            iter->second->abort_writes();
        }
        for (std::map<std::string, tokyo::Tree_db *>::reverse_iterator iter = fields_tree_.rbegin();
             fields_tree_.rend() != iter;
             ++iter)
        {
            iter->second->abort_writes();
        }
    }
    
    Bson* Storage::configuration()
    {
        return config_;
    }
    
    const std::string& Storage::name() const
    {
        return name_;
    }
    
    //======================================================================
    // Storage configuration methods.
    
    void storage_config_init(lj::Bson& cfg,
                             const std::string& name)
    {
        cfg.set_child("main/name", lj::bson_new_string(name));
        cfg.set_child("main/compare", lj::bson_new_string("int64"));
        cfg.set_child("main/file", lj::bson_new_string(std::string("db.") + name + ".tcb"));
        cfg.set_child("main/backup_file", lj::bson_new_string(std::string("db.") + name + ".tcb.backup"));
        cfg.set_child("journal/file", lj::bson_new_string(std::string("journal.") + name + ".tcf"));
        cfg.set_child("journal/type", lj::bson_new_string("fixed"));
    }
    
    void storage_config_add_index(lj::Bson& cfg,
                                  const std::string& type,
                                  const std::string& field,
                                  const std::string& comp)
    {
        std::string extension;
        if (type.compare("tree") == 0)
        {
            extension = "tcb";
        }
        else if (type.compare("hash") == 0)
        {
            extension = "tch";
        }
        else if (type.compare("text") == 0)
        {
            extension = "tcq";
        }
        else if (type.compare("tag") == 0)
        {
            extension = "tcw";
        }
        else {
            extension = type + ".tc";
        }
        
        std::string name;
        for (std::string::const_iterator iter = field.begin();
             field.end() != iter;
             ++iter)
        {
            if ('/' == *iter)
            {
                // We push an escae before the slash to avoid confusion in set_child.
                name.push_back('~');
            }
            else
            {
                name.push_back(*iter);
            }
        }
        
        lj::Bson* index_cfg = new lj::Bson();
        cfg.set_child(std::string("index/") + type + "/" + name,
                      index_cfg);
        
        index_cfg->set_child("compare", lj::bson_new_string(comp));
        index_cfg->set_child("file", lj::bson_new_string(std::string("index.") + name + "." + extension));
        index_cfg->set_child("type", lj::bson_new_string(type));
        index_cfg->set_child("field", lj::bson_new_string(field));
    }
    
    void storage_config_remove_index(lj::Bson& cfg,
                                     const std::string& type,
                                     const std::string& field)
    {
        std::string name;
        for (std::string::const_iterator iter = field.begin();
             field.end() != iter;
             ++iter)
        {
            if ('/' == *iter)
            {
                // We push an escae before the slash to avoid confusion in set_child.
                name.push_back('~');
            }
            else
            {
                name.push_back(*iter);
            }
        }
        
        lj::Bson* index_cfg = cfg.path(std::string("index/") + type + "/" + name);
        index_cfg->destroy();
    }
    
    void storage_config_add_subfield(lj::Bson& cfg,
                                     const std::string& field)
    {
        lj::Bson* ptr = cfg.path("main/nested");
        std::set<std::string> allowed(lj::bson_as_value_string_set(*ptr));
        allowed.insert(field);
        
        ptr->destroy();
        for (std::set<std::string>::const_iterator iter = allowed.begin();
             allowed.end() != iter;
             ++iter)
        {
            ptr->push_child("", lj::bson_new_string(*iter));
        }
    }
    
    void storage_config_save(const lj::Bson& cfg, const lj::Bson& server_config)
    {
        std::string dbfile(root_data_directory(server_config));
        
        const std::string& dbname = lj::bson_as_string(cfg.nav("main/name"));
        dbfile.append("/").append(dbname);
        
        // XXX This should be moved somewhere for portability.
        int err = mkdir(dbfile.c_str(), S_IRWXU | S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP);
        if (0 != err && EEXIST != errno)
        {
            throw new lj::Exception("StorageConfigSave", strerror(errno));
        }
        
        dbfile.append("/config");
        lj::bson_save(cfg, dbfile);
    }
    
    lj::Bson* storage_config_load(const std::string& dbname, std::string dbfile)
    {
        dbfile.append("/").append(dbname).append("/config");
        lj::Bson* ptr = lj::bson_load(dbfile);
        return ptr;
    }
};