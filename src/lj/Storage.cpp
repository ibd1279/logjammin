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

namespace lj
{
    //=====================================================================
    // Storage Implementation.
    //=====================================================================
    
    namespace
    {
        void storage_tree_cfg(TCBDB* db, const void* ptr)
        {
            const Bson* bn = static_cast<const Bson*>(ptr);
            if (lj::bson_as_string(bn->nav("compare")).compare("lex") == 0)
            {
                tcbdbsetcmpfunc(db, tcbdbcmplexical, NULL);
                Log::info.log("Using lexical for compares") << Log::end;
            }
            else if (lj::bson_as_string(bn->nav("compare")).compare("int32") == 0)
            {
                tcbdbsetcmpfunc(db, tcbdbcmpint32, NULL);
                Log::info.log("Using int32 for compares") << Log::end;
            }
            else
            {
                tcbdbsetcmpfunc(db, tcbdbcmpint64, NULL);
                Log::info.log("Using int64 for compares") << Log::end;
            }
            tcbdbtune(db, 256, 512, 65498, 9, 11, BDBTLARGE | BDBTBZIP);
        }
        
        void storage_hash_cfg(TCHDB* db, const void* ptr)
        {
            //const Bson *bn = static_cast<const Bson *>(ptr);
            tchdbtune(db, 514229, 8, 11, HDBTLARGE | HDBTBZIP);
            
            // XXX config other things like compression type.
        }
        
        void storage_text_cfg(TCQDB* db, const void* ptr)
        {
            //const Bson *bn = static_cast<const Bson *>(ptr);
            tcqdbtune(db, 10000000, QDBTLARGE | QDBTBZIP);
            
            // XXX config other things like compression type.
        }
        
        // XXX Add configuration method for tags
        void storage_tag_cfg(TCWDB* db, const void* ptr)
        {
            //const Bson *bn = static_cast<const Bson *>(ptr);
            tcwdbtune(db, 10000000, WDBTLARGE | WDBTBZIP);
            
            // XXX config other things like compression type.
        }
        
        void storage_journal_cfg(TCFDB* db, const void* ptr)
        {
            tcfdbtune(db, sizeof(bool), -1);
        }
        
        template<typename T>
        void open_storage_index(const std::string& dir,
                                const Linked_map<std::string, Bson*>& cfg,
                                int open_flags,
                                typename T::Tune_function_pointer tune_function,
                                std::map<std::string, T*>& dest)
        {
            for (lj::Linked_map<std::string, Bson*>::const_iterator iter = cfg.begin();
                 cfg.end() != iter;
                 ++iter)
            {
                if (!(*iter).second->nav("file").exists() ||
                    !(*iter).second->nav("field").exists())
                {
                    Log::error.log("Unable to open index [%s] because file or field is not set.") << (*iter).first << Log::end; 
                    continue;
                }
                std::string indexfile(dir + "/" + bson_as_string((*iter).second->nav("file")));
                std::string field = bson_as_string((*iter).second->nav("field"));
                Log::debug.log("  Opening [%s] for [%s]") << indexfile << field << Log::end;
                T* db = new T(indexfile,
                              open_flags,
                              tune_function,
                              (*iter).second);
                dest.insert(std::pair<std::string, T*>(field, db));
            }
        }
        
        std::pair<int, int> bson_to_storage_delta(const lj::Bson* ptr)
        {
            if(lj::bson_type_is_quotable(ptr->type()))
            {
                return std::pair<int, int>(4,5);
            }
            else
            {
                return std::pair<int, int>(0,0);
            }
        }
        
        void tree_deindex(tokyo::Tree_db* db,
                          const lj::Bson* n,
                          const unsigned long long key)
        {
            char* bson = n->to_binary();
            std::pair<int, int> delta(bson_to_storage_delta(n));
            db->remove_from_existing(bson + delta.first,
                                     n->size() - delta.second,
                                     &key,
                                     sizeof(unsigned long long));
            delete[] bson;
        }
        
        void tree_reindex(tokyo::Tree_db* db,
                          const lj::Bson* n,
                          const unsigned long long key)
        {
            char* bson = n->to_binary();
            std::pair<int, int> delta(bson_to_storage_delta(n));
            db->remove_from_existing(bson + delta.first,
                                     n->size() - delta.second,
                                     &key,
                                     sizeof(unsigned long long));
            delete[] bson;
        }
        
        void hash_deindex(tokyo::Hash_db* db,
                          const lj::Bson* n,
                          const unsigned long long key)
        {
            char* bson = n->to_binary();
            std::pair<int, int> delta(bson_to_storage_delta(n));
            db->remove(bson + delta.first,
                       n->size() - delta.second);
            delete[] bson;
        }
        
        void hash_reindex(tokyo::Hash_db* db,
                          const lj::Bson* n,
                          const unsigned long long key)
        {
            char* bson = n->to_binary();
            std::pair<int, int> delta(bson_to_storage_delta(n));
            db->place(bson + delta.first,
                      n->size() - delta.second,
                      &key,
                      sizeof(unsigned long long));
            delete[] bson;
        }
        
        void text_deindex(tokyo::TextSearcher* db,
                          const lj::Bson* n,
                          const unsigned long long key)
        {
            db->remove(key, lj::bson_as_string(*n));
        }
        
        void text_reindex(tokyo::TextSearcher* db,
                          const lj::Bson* n,
                          const unsigned long long key)
        {
            db->index(key, bson_as_string(*n));
        }
        
        void tag_deindex(tokyo::TagSearcher* db,
                         const lj::Bson* n,
                         const unsigned long long key)
        {
            db->remove(key, bson_as_value_string_set(*n));
        }
        
        void tag_reindex(tokyo::TagSearcher* db,
                         const lj::Bson* n,
                         const unsigned long long key)
        {
            db->index(key, bson_as_value_string_set(*n));
        }
        
        template<typename T>
        void execute_all_indicies(const std::map<std::string, T*>& m,
                                  const std::string& indextype,
                                  const bool allow_subfields,
                                  const std::set<std::string>& subfields,
                                  const lj::Bson& record,
                                  const unsigned long long key,
                                  void (*func)(T* db,
                                               const lj::Bson* n,
                                               const unsigned long long key))
        {
            typedef typename std::map<std::string, T*>::const_iterator index_map_iter;
            // Remove from tree index entries.
            for (index_map_iter iter = m.begin();
                 m.end() != iter;
                 ++iter)
            {
                const lj::Bson* n = record.path(iter->first);
                if (!n)
                {
                    continue;
                }
                if (!n->exists())
                {
                    continue;
                }
                
                if (lj::bson_type_is_nested(n->type()) && 
                    subfields.end() != subfields.find(iter->first) &&
                    allow_subfields)
                {
                    Log::debug.log("  Deindex [%d] from [%s] nested %s index.") << key << iter->first << indextype << Log::end;
                    for (Linked_map<std::string, Bson*>::const_iterator iter2 = n->to_map().begin();
                         n->to_map().end() != iter2;
                         ++iter2)
                    {
                        func(iter->second, iter2->second, key);
                    }
                }
                else
                {
                    Log::debug.log("  Deindex [%d] from [%s] %s index.") << key << iter->first << indextype << Log::end;
                    func(iter->second, n, key);
                }
            }
        }
        
        template<typename T>
        void truncate_all_indicies(const std::map<std::string, T*>& m)
        {
            typedef typename std::map<std::string, T*>::const_iterator index_map_iter;
            for (index_map_iter iter = m.begin();
                 m.end() != iter;
                 ++iter)
            {
                (*iter).second->truncate();
            }
        }
    }; // namespace
    
    Storage::Storage(const std::string &name) : db_(NULL), fields_tree_(), fields_hash_(), fields_text_(), fields_tag_(), nested_indexing_(), config_(NULL), name_(name)
    {
        std::string dir(directory());
        std::string configfile(dir + "/config");
        
        Log::info.log("Loading configuration from [%s].") << configfile << Log::end;
        config_ = bson_load(configfile);
        Log::info.log("Loaded Settings [%s].") << lj::bson_as_pretty_string(*config_) << Log::end;
        
        std::string dbfile(dir + "/" + lj::bson_as_string(config_->nav("main/file")));
        Log::info.log("Opening database [%s].") << dbfile << Log::end;
        db_ = new tokyo::Tree_db(dbfile,
                                 BDBOREADER | BDBOWRITER | BDBOCREAT,
                                 &storage_tree_cfg,
                                 config_->path("main"));
        
        std::string journalfile(dir + "/" + lj::bson_as_string(config_->nav("journal/file")));
        Log::info.log("Opening journal [%s].") << journalfile << Log::end;
        journal_ = new tokyo::Fixed_db(journalfile,
                                       FDBOREADER | FDBOWRITER | FDBOCREAT,
                                       &storage_journal_cfg,
                                       config_->path("journal"));
        
        Log::info.log("Opening tree indices under [%s].") << dir << Log::end;
        open_storage_index<tokyo::Tree_db>(dir,
                                           config_->nav("index/tree").to_map(),
                                           BDBOREADER | BDBOWRITER | BDBOCREAT,
                                           &storage_tree_cfg,
                                           fields_tree_);
        
        Log::info.log("Opening hash indices under [%s].") << dir << Log::end;
        open_storage_index<tokyo::Hash_db>(dir,
                                           config_->nav("index/hash").to_map(),
                                           HDBOREADER | HDBOWRITER | HDBOCREAT,
                                           &storage_hash_cfg,
                                           fields_hash_);
        
        Log::info.log("Opening text indices under [%s].") << dir << Log::end;
        open_storage_index<TextSearcher>(dir,
                                         config_->nav("index/text").to_map(),
                                         QDBOREADER | QDBOWRITER | QDBOCREAT,
                                         &storage_text_cfg,
                                         fields_text_);
        
        Log::info.log("Opening tag indices under [%s].") << dir << Log::end;
        open_storage_index<TagSearcher>(dir,
                                        config_->nav("index/tag").to_map(),
                                        WDBOREADER | WDBOWRITER | WDBOCREAT,
                                        &storage_tag_cfg,
                                        fields_tag_);
        
        Log::info.log("Registering nested indexing from [%s].") << dir << Log::end;
        Bson *nested_fields = config_->path("main/nested");
        for (Linked_map<std::string, Bson*>::const_iterator iter = nested_fields->to_map().begin();
             nested_fields->to_map().end() != iter;
             ++iter)
        {
            Log::info.log("Adding nested field [%s].") << lj::bson_as_string(*iter->second) << Log::end;
            nested_indexing_.insert(lj::bson_as_string(*iter->second));
        }
        
        Log::info.log("Checkpointing after startup.") << Log::end;
        checkpoint();
    }
    
    Storage::~Storage()
    {
        Log::info.log("Checkpointing before shutdown.") << Log::end;
        checkpoint();
        
        std::string dir(directory());
        if (fields_tag_.size())
        {
            Log::info.log("Closing tag indicies under [%s].") << dir << Log::end;
            for (std::map<std::string, TagSearcher *>::const_iterator iter = fields_tag_.begin();
                 fields_tag_.end() != iter;
                 ++iter)
            {
                Log::info.log("Closing tag index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        if (fields_text_.size())
        {
            Log::info.log("Closing text indicies under [%s].") << dir << Log::end;
            for (std::map<std::string, TextSearcher *>::const_iterator iter = fields_text_.begin();
                 fields_text_.end() != iter;
                 ++iter)
            {
                Log::info.log("Closing text index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        if (fields_hash_.size())
        {
            Log::info.log("Closing hash indicies under [%s].") << dir << Log::end;
            for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
                 fields_hash_.end() != iter;
                 ++iter)
            {
                Log::info.log("Closing hash index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        if (fields_tree_.size())
        {
            Log::info.log("Closing tree indicies under [%s].") << dir << Log::end;
            for(std::map<std::string, tokyo::Tree_db *>::const_iterator iter = fields_tree_.begin();
                iter != fields_tree_.end();
                ++iter) {
                Log::info.log("Closing tree index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        Log::info.log("Closing journal for [%s].") << dir << Log::end;
        if (journal_)
        {
            delete journal_;
            journal_ = 0;
        }
        
        Log::info.log("Closing database for [%s].") << dir << Log::end;
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
                Log::debug.log("  Repairing [%d].") << key << Log::end;
                lj::Bson record;
                at(key)->first(record);
                deindex(record);
                reindex(record);
            }
        }
        
        Log::debug.log("  Clearing journal.") << Log::end;
        journal_->truncate();
        
        if (modified)
        {
            Log::debug.log("  Backing up datafile.") << Log::end;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            std::ostringstream target;
            target << directory() << "/";
            target << lj::bson_as_string(config_->nav("main/file")) << ".backup";
            target << "." << (tv.tv_sec);
            db_->copy(target.str());
        }
    }
    
    void Storage::rebuild()
    {
        Log::info.log("Truncating all indicies in [%s]") << directory() << Log::end;
        truncate_all_indicies<tokyo::Tree_db>(fields_tree_);
        truncate_all_indicies<tokyo::Hash_db>(fields_hash_);
        truncate_all_indicies<tokyo::TextSearcher>(fields_text_);
        truncate_all_indicies<tokyo::TagSearcher>(fields_tag_);
        
        Log::info.log("Rebuilding all indicies in [%s]") << directory() << Log::end;
        tokyo::Tree_db::Enumerator* e = db_->forward_enumerator();
        while (e->more())
        {
            tokyo::DB::value_t v = e->next();
            lj::Bson* record = new lj::Bson(lj::Bson::k_document,
                                            static_cast<char*>(v.first));
            free(v.first);
            reindex(*record);
        }
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
            journal_end(key);
        }
        catch(Exception* ex)
        {
            deindex(value);
            value.nav("__key").set_value(Bson::k_int64, reinterpret_cast<char *>(&original_key));
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
                deindex(value);
                db_->remove(&key, sizeof(unsigned long long));
                journal_end(key);
                value.nav("__key").destroy();
            }
            catch (Exception* ex)
            {
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
                std::pair<int, int> delta(bson_to_storage_delta(iter->second));
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
            std::pair<int, int> delta(bson_to_storage_delta(&n));
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
        execute_all_indicies<tokyo::Tree_db>(fields_tree_,
                                             "tree",
                                             true,
                                             nested_indexing_,
                                             record,
                                             key,
                                             &tree_deindex);
        
        execute_all_indicies<tokyo::Hash_db>(fields_hash_,
                                             "hash",
                                             true,
                                             nested_indexing_,
                                             record,
                                             key,
                                             &hash_deindex);
        
        execute_all_indicies<tokyo::TextSearcher>(fields_text_,
                                                  "text",
                                                  false,
                                                  nested_indexing_,
                                                  record,
                                                  key,
                                                  &text_deindex);
        execute_all_indicies<tokyo::TagSearcher>(fields_tag_,
                                                 "word",
                                                 false,
                                                 nested_indexing_,
                                                 record,
                                                 key,
                                                 &tag_deindex);
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
        
        Log::debug.log("Index [%d].") << key << Log::end;
        execute_all_indicies<tokyo::Tree_db>(fields_tree_,
                                             "tree",
                                             true,
                                             nested_indexing_,
                                             record,
                                             key,
                                             &tree_reindex);
        
        execute_all_indicies<tokyo::Hash_db>(fields_hash_,
                                             "hash",
                                             true,
                                             nested_indexing_,
                                             record,
                                             key,
                                             &hash_reindex);
        
        execute_all_indicies<tokyo::TextSearcher>(fields_text_,
                                                  "text",
                                                  false,
                                                  nested_indexing_,
                                                  record,
                                                  key,
                                                  &text_reindex);
        execute_all_indicies<tokyo::TagSearcher>(fields_tag_,
                                                 "word",
                                                 false,
                                                 nested_indexing_,
                                                 record,
                                                 key,
                                                 &tag_reindex);
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
    
    std::string Storage::directory()
    {
        std::string dir(DBDIR);
        dir.append("/").append(name());
        return dir;
    }
    
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
    
    void storage_config_save(lj::Bson& cfg)
    {
        std::string dbname = lj::bson_as_string(cfg.nav("main/name"));
        std::string dbfile(DBDIR);
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
    
    lj::Bson* storage_config_load(const std::string& dbname)
    {
        std::string dbfile(DBDIR);
        dbfile.append("/").append(dbname).append("/config");
        lj::Bson* ptr = lj::bson_load(dbfile);
        return ptr;
    }
};