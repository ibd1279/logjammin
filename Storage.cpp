/*
 \file Storage.cpp
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

#include "build/default/config.h"
#include <iostream>
#include "Storage.h"
#include "Exception.h"
#include "Logger.h"

using tokyo::Hash_db;
using tokyo::TreeDB;
using tokyo::TextSearcher;
using tokyo::TagSearcher;

namespace lj {
    
    namespace {
        void dbvalue_to_storagekey(const tokyo::DB::list_value_t &ptr,
                                   std::set<unsigned long long> &keys) {
            for (tokyo::DB::list_value_t::const_iterator iter = ptr.begin();
                 ptr.end() != iter;
                 ++iter)
            {
                unsigned long long *x = static_cast<unsigned long long *>(iter->first);
                if (x)
                {
                    keys.insert(*x);
                    free(iter->first);
                }
            }
        }
    };
    
    //=====================================================================
    // Storage Filter Implementation.
    //=====================================================================
    
    BSONNode StorageFilter::doc_at(unsigned long long pkey) const {
        return _storage->at(pkey);
    }
    
    StorageFilter::StorageFilter(const Storage *storage,
                                 const std::set<unsigned long long> &keys,
                                 StorageFilterMode mode,
                                 long long offset,
                                 long long length) : _storage(storage), _keys(keys), _mode(mode), _offset(offset), _length(length) {
    }
    
    StorageFilter::StorageFilter(const StorageFilter &orig) : _storage(orig._storage), _keys(orig._keys), _mode(orig._mode), _offset(orig._offset), _length(orig._length) {
    }
    
    StorageFilter::~StorageFilter() {
        _storage = NULL;
    }
    
    bool StorageFilter::contains(unsigned long long key) const {
        return _keys.find(key) != _keys.end();
    }
    
    StorageFilter &StorageFilter::union_keys(const std::set<unsigned long long> &keys) {
        for(std::set<unsigned long long>::const_iterator iter = keys.begin();
            iter != keys.end();
            ++iter) {
            _keys.insert(*iter);
        }
        return *this;
    }
    
    StorageFilter &StorageFilter::intersect_keys(const std::set<unsigned long long> &keys) {
        std::set<unsigned long long>::iterator iter = _keys.begin();
        while(iter != _keys.end()) {
            if(keys.find(*iter) == keys.end()) {
                _keys.erase(iter++);
            } else {
                ++iter;
            }
        }
        return *this;
    }
    
    StorageFilter &StorageFilter::refine(const std::string &indx,
                                         const void * const val,
                                         const size_t val_len) {
        Log::debug.log("Filtering on [%s] with [%d][%s].") << indx << ((unsigned long long)val_len) << ((char *)val) << Log::end;
        std::map<std::string, TreeDB*>::const_iterator tree_index = _storage->fields_tree_.find(indx);
        std::map<std::string, Hash_db*>::const_iterator hash_index = _storage->fields_hash_.find(indx);
        
        tokyo::DB::list_value_t db_values;
        if (_storage->fields_tree_.end() != tree_index)
        {
            tree_index->second->at_together(val, val_len, db_values);
        }
        else if (_storage->fields_hash_.end() != hash_index)
        {
            db_values.push_back(hash_index->second->at(val, val_len));
        }
        else
        {
            return *this;
        }
        
        std::set<unsigned long long> storage_keys;
        dbvalue_to_storagekey(db_values, storage_keys);
        
        switch (_mode)
        {
            case INTERSECTION:
                intersect_keys(storage_keys);
                break;
            case UNION:
                union_keys(storage_keys);
                break;
        }
        return *this;
    }
    
    StorageFilter &StorageFilter::search(const std::string &indx,
                                         const std::string &terms) {
        Log::debug.log("Searching on [%s] with [%s]") << indx << terms << Log::end;
        std::map<std::string, TextSearcher *>::const_iterator index = _storage->fields_text_.find(indx);
        if(index == _storage->fields_text_.end())
            return *this;
        
        TextSearcher *searcher = index->second;
        tokyo::Searcher::set_key_t searcher_values;
        searcher->search(terms, searcher_values);
        
        switch(_mode) {
            case INTERSECTION:
                intersect_keys(searcher_values);
                break;
            case UNION:
                union_keys(searcher_values);
                break;
        }
        return *this;
    }
    
    StorageFilter &StorageFilter::tagged(const std::string &indx,
                                         const std::string &word) {
        Log::debug.log("Searching on [%s] with [%s]") << indx << word << Log::end;
        std::map<std::string, TagSearcher *>::const_iterator index = _storage->fields_tag_.find(indx);
        if(index == _storage->fields_tag_.end())
            return *this;
        
        TagSearcher *searcher = index->second;
        tokyo::Searcher::set_key_t searcher_values;
        searcher->search(word, searcher_values);
        
        switch(_mode) {
            case INTERSECTION:
                intersect_keys(searcher_values);
                break;
            case UNION:
                intersect_keys(searcher_values);
                break;
        }
        return *this;
    }
    
    //=====================================================================
    // Storage Implementation.
    //=====================================================================
    
    namespace
    {
        void storage_tree_cfg(TCBDB* db, const void* ptr)
        {
            const BSONNode* bn = static_cast<const BSONNode*>(ptr);
            if (bn->nav("compare").to_s().compare("lex") == 0)
            {
                tcbdbsetcmpfunc(db, tcbdbcmplexical, NULL);
                Log::info.log("Using lexical for compares") << Log::end;
            }
            else if (bn->nav("compare").to_s().compare("int32") == 0)
            {
                tcbdbsetcmpfunc(db, tcbdbcmpint32, NULL);
                Log::info.log("Using int32 for compares") << Log::end;
            }
            else
            {
                tcbdbsetcmpfunc(db, tcbdbcmpint64, NULL);
                Log::info.log("Using int64 for compares") << Log::end;
            }
            
            // XXX config other things like compression type, tree hight, etc.
        }
        
        void storage_hash_cfg(TCHDB* db, const void* ptr)
        {
            //const BSONNode *bn = static_cast<const BSONNode *>(ptr);
            
            // XXX config other things like compression type.
        }
        
        void storage_text_cfg(TCQDB* db, const void* ptr)
        {
            //const BSONNode *bn = static_cast<const BSONNode *>(ptr);
            
            // XXX config other things like compression type.
        }
        
        // XXX Add configuration method for tags
        void storage_tag_cfg(TCWDB* db, const void* ptr)
        {
            //const BSONNode *bn = static_cast<const BSONNode *>(ptr);
            
            // XXX config other things like compression type.
        }
        
        template<typename T, typename Q>
        void open_storage_index(const std::string& dir,
                                const lj::BSONNode::childmap_t& cfg,
                                int open_flags,
                                void (*tune_function)(Q*, const void*),
                                std::map<std::string, T*>& dest)
        {
            for (lj::BSONNode::childmap_t::const_iterator iter = cfg.begin();
                 iter != cfg.end();
                 ++iter)
            {
                if (!(*iter).second->nav("file").exists() ||
                    !(*iter).second->nav("field").exists())
                {
                    Log::error.log("Unable to open index [%s] because file or field is not set.") << (*iter).first << Log::end; 
                    continue;
                }
                std::string indexfile(dir + "/" + (*iter).second->nav("file").to_s());
                T* db = new T(indexfile,
                              open_flags,
                              tune_function,
                              (*iter).second);
                dest.insert(std::pair<std::string, T*>((*iter).second->nav("field").to_s(), db));
            }
        }
        
        std::pair<int, int> bson_to_storage_delta(const lj::BSONNode* ptr)
        {
            if(ptr->quotable())
            {
                return std::pair<int, int>(4,5);
            }
            else
            {
                return std::pair<int, int>(0,0);
            }
        }
    } // namespace
    
    Storage::Storage(const std::string &dir) : db_(NULL), fields_tree_(), fields_hash_(), fields_text_(), fields_tag_(), _fields_unique(), directory_(DBDIR)
    {
        directory_.append("/").append(dir);
        std::string configfile(directory_ + "/config");
        
        Log::info.log("Loading configuration from [%s].") << configfile << Log::end;
        BSONNode cfg;
        cfg.load(configfile);
        Log::info.log("Loaded Settings [%s].") << cfg.to_pretty_s() << Log::end;
        
        std::string dbfile(directory_ + "/" + cfg.nav("main/file").to_s());
        Log::info.log("Opening database [%s].") << dbfile << Log::end;
        db_ = new TreeDB(dbfile,
                         BDBOREADER | BDBOWRITER | BDBOCREAT,
                         &storage_tree_cfg,
                         &(cfg.nav("main")));
        
        Log::info.log("Opening tree indices under [%s].") << directory_ << Log::end;
        open_storage_index<TreeDB, TCBDB>(directory_,
                                          cfg.nav("index/tree").to_map(),
                                          BDBOREADER | BDBOWRITER | BDBOCREAT,
                                          &storage_tree_cfg,
                                          fields_tree_);
        
        Log::info.log("Opening hash indices under [%s].") << directory_ << Log::end;
        open_storage_index<Hash_db, TCHDB>(directory_,
                                           cfg.nav("index/hash").to_map(),
                                           HDBOREADER | HDBOWRITER | HDBOCREAT,
                                           &storage_hash_cfg,
                                           fields_hash_);
        
        Log::info.log("Opening text indices under [%s].") << directory_ << Log::end;
        open_storage_index<TextSearcher, TCQDB>(directory_,
                                                cfg.nav("index/text").to_map(),
                                                QDBOREADER | QDBOWRITER | QDBOCREAT,
                                                &storage_text_cfg,
                                                fields_text_);
        
        Log::info.log("Opening tag indices under [%s].") << directory_ << Log::end;
        open_storage_index<TagSearcher, TCWDB>(directory_,
                                               cfg.nav("index/tag").to_map(),
                                               WDBOREADER | WDBOWRITER | WDBOCREAT,
                                               &storage_tag_cfg,
                                               fields_tag_);
        
        Log::info.log("Registering unique fields from [%s].") << directory_ << Log::end;
        for (BSONNode::childmap_t::const_iterator iter = cfg.nav("main/unique").to_map().begin();
             iter != cfg.nav("main/unique").to_map().end();
             ++iter)
        {
            Log::info.log("Adding unique field [%s].") << iter->second->to_s() << Log::end;
            _fields_unique.insert(iter->second->to_s());
        }
    }
    
    Storage::~Storage() {
        Log::info.log("Closing tag indicies under [%s].") << directory_ << Log::end;
        for(std::map<std::string, TagSearcher *>::const_iterator iter = fields_tag_.begin();
            iter != fields_tag_.end();
            ++iter) {
            Log::info.log("Closing tag index for field [%s].") << iter->first << Log::end;
            delete iter->second;
        }
        Log::info.log("Closing text indicies under [%s].") << directory_ << Log::end;
        for(std::map<std::string, TextSearcher *>::const_iterator iter = fields_text_.begin();
            iter != fields_text_.end();
            ++iter) {
            Log::info.log("Closing text index for field [%s].") << iter->first << Log::end;
            delete iter->second;
        }
        
        Log::info.log("Closing hash indicies under [%s].") << directory_ << Log::end;
        for (std::map<std::string, Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            Log::info.log("Closing hash index for field [%s].") << iter->first << Log::end;
            delete iter->second;
        }
        
        Log::info.log("Closing tree indicies under [%s].") << directory_ << Log::end;
        for(std::map<std::string, TreeDB *>::const_iterator iter = fields_tree_.begin();
            iter != fields_tree_.end();
            ++iter) {
            Log::info.log("Closing tree index for field [%s].") << iter->first << Log::end;
            delete iter->second;
        }
        
        Log::info.log("Closing database for field [%s].") << directory_ << Log::end;
        delete db_;
    }
    
    BSONNode Storage::at(const unsigned long long key) const
    {
        tokyo::DB::value_t p = db_->at(&key, sizeof(unsigned long long));
        if (!p.first)
        {
            return BSONNode();
        }
        BSONNode n(DOC_NODE, (char *)p.first);
        free(p.first);
        return n;
    }
    
    StorageFilter Storage::all() const
    {
        tokyo::DB::list_value_t keys;
        tokyo::DB::value_t max = db_->max_key();
        tokyo::DB::value_t min = db_->min_key();
        if (db_->range_keys(min.first,
                            min.second,
                            true,
                            max.first,
                            max.second,
                            true,
                            keys))
        {
            std::set<unsigned long long> real_keys;
            dbvalue_to_storagekey(keys, real_keys);
            return StorageFilter(this, real_keys);
        }
        return StorageFilter(this, std::set<unsigned long long>());
    }
    
    StorageFilter Storage::none() const
    {
        return StorageFilter(this, std::set<unsigned long long>());
    }
    
    StorageFilter Storage::refine(const std::string &indx,
                                  const void * const val,
                                  const size_t val_len) const
    {
        Log::debug.log("Filtering on [%s] with [%d][%s].") << indx << val_len << ((char *)val) << Log::end;
        std::map<std::string, TreeDB*>::const_iterator tree_index = fields_tree_.find(indx);
        std::map<std::string, Hash_db*>::const_iterator hash_index = fields_hash_.find(indx);
        
        tokyo::DB::list_value_t db_values;
        if (fields_tree_.end() != tree_index)
        {
            tree_index->second->at_together(val, val_len, db_values);
        }
        else if (fields_hash_.end() != hash_index)
        {
            db_values.push_back(hash_index->second->at(val, val_len));
        }
        else
        {
            return none();
        }
        
        std::set<unsigned long long> storage_keys;
        dbvalue_to_storagekey(db_values, storage_keys);
        return StorageFilter(this, storage_keys);
    }
    
    StorageFilter Storage::search(const std::string &indx,
                                  const std::string &terms) const {
        Log::debug.log("Searching on [%s] with [%s]") << indx << terms << Log::end;
        std::map<std::string, TextSearcher *>::const_iterator index = fields_text_.find(indx);
        if(index == fields_text_.end()) {
            Log::warning.log("Request for unknown text index [%s] from [%s].") << indx << directory_ << Log::end;
            return none();
        }
        
        TextSearcher *searcher = index->second;
        tokyo::Searcher::set_key_t searcher_values;
        searcher->search(terms, searcher_values);
        
        return StorageFilter(this, searcher_values);
    }
    
    StorageFilter Storage::tagged(const std::string &indx,
                                  const std::string &word) const {
        Log::debug.log("Searching on [%s] with [%s]") << indx << word << Log::end;
        std::map<std::string, TagSearcher *>::const_iterator index = fields_tag_.find(indx);
        if(index == fields_tag_.end()) {
            Log::warning.log("Request for unknown tag index [%s] from [%s].") << indx << directory_ << Log::end;
            return none();
        }
        
        TagSearcher *searcher = index->second;
        tokyo::Searcher::set_key_t searcher_values;
        searcher->search(word, searcher_values);
        
        return StorageFilter(this, searcher_values);
    }
    
    Storage &Storage::place(BSONNode &value)
    {
        unsigned long long key = value.nav("__key").to_l();
        unsigned long long original_key = value.nav("__key").to_l();
        
        Log::debug.log("Placing [%llu] [%s]") << key << value.to_pretty_s() << Log::end;
        try
        {
            begin_transaction();
            if (key)
            {
                Log::debug.log("Deindexing previous record to clean house.") << Log::end;
                deindex(key);
            }
            else
            {
                Log::debug.log("New record. calculating key.") << Log::end;
                unsigned long long *ptr = static_cast<unsigned long long *>(db_->max_key().first);
                key = (*ptr) + 1;
                free(ptr);
                Log::debug.log("New key is [%d].") << key << Log::end;
            }
            
            // Enforce unique constraints.
            Log::debug.log("Unique constraint check.") << Log::end;
            for (std::map<std::string, Hash_db*>::const_iterator iter = fields_hash_.begin();
                 fields_hash_.end() != iter;
                 ++iter)
            {
                BSONNode n(value.nav(iter->first));
                if (n.exists())
                {
                    check_unique(n, iter->first, iter->second);
                }
            }
            
            Log::debug.log("Place in DB.") << Log::end;
            value.nav("__key").value(static_cast<long long>(key));
            char* bson = value.bson();
            db_->place(&key,
                       sizeof(unsigned long long),
                       bson,
                       value.size());
            delete[] bson;
            
            reindex(key);
            commit_transaction();
        }
        catch(Exception* ex)
        {
            value.nav("__key").value(static_cast<long long>(original_key));
            abort_transaction();
            throw ex;
        }
        return *this;
    }
    
    Storage &Storage::remove(BSONNode &value)
    {
        unsigned long long key = value.nav("__key").to_l();
        
        Log::debug.log("Removing [%llu] [%s]") << key << value.to_pretty_s() << Log::end;
        
        if (key)
        {
            try
            {
                begin_transaction();
                deindex(key);
                db_->remove(&key, sizeof(unsigned long long));
                commit_transaction();
                value.nav("__key").value(0LL);
            }
            catch (Exception* ex)
            {
                abort_transaction();
                throw ex;
            }
        }
        return *this;
    }
    
    Storage &Storage::check_unique(const BSONNode &n, const std::string &name, tokyo::DB *index)
    {
        if (n.nested() && _fields_unique.end() != _fields_unique.find(name))
        {
            Log::debug.log("checking children of [%s].") << name << Log::end;
            for (BSONNode::childmap_t::const_iterator iter = n.to_map().begin();
                 iter != n.to_map().end();
                 ++iter)
            {
                char* bson = iter->second->bson();
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
            char *bson = n.bson();
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
    
    // XXX Method is too long. Needs to be devided up somehow.
    Storage &Storage::deindex(const unsigned long long key)
    {
        if (!key)
        {
            return *this;
        }
        
        Log::debug.log("Remove [%d] from indicies.") << key << Log::end;
        BSONNode original = at(key);
        
        // Remove from tree index entries.
        for (std::map<std::string, TreeDB *>::const_iterator iter = fields_tree_.begin();
             fields_tree_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists() && 
                n.nested() && 
                _fields_unique.end() != _fields_unique.find(iter->first))
            {
                for (BSONNode::childmap_t::const_iterator iter2 = n.to_map().begin();
                     iter2 != n.to_map().end();
                     ++iter2)
                {
                    char* bson = iter2->second->bson();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->remove_from_existing(bson + delta.first,
                                                       iter2->second->size() - delta.second,
                                                       &key,
                                                       sizeof(unsigned long long));
                    delete[] bson;
                }
            }
            else if (n.exists())
            {
                char* bson = n.bson();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->remove_from_existing(bson + delta.first,
                                                   n.size() - delta.second,
                                                   &key,
                                                   sizeof(unsigned long long));
                delete[] bson;
            }
        }
        
        // remove from hash index entries.
        for (std::map<std::string, Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists() && 
                n.nested() && 
                _fields_unique.end() != _fields_unique.find(iter->first))
            {
                for (BSONNode::childmap_t::const_iterator iter2 = n.to_map().begin();
                     iter2 != n.to_map().end();
                     ++iter2)
                {
                    char* bson = iter2->second->bson();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->remove(bson + delta.first,
                                         iter2->second->size() - delta.second);
                    delete[] bson;
                }
            }
            else if (n.exists())
            {
                char* bson = n.bson();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->remove(bson + delta.first,
                                     n.size() - delta.second);
                delete[] bson;
            }
        }
        
        // remove from text searches.
        for (std::map<std::string, TextSearcher *>::const_iterator iter = fields_text_.begin();
             fields_text_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->remove(key, n.to_s());
            }
        }
        
        // remove from the tag searches.
        for (std::map<std::string, TagSearcher *>::const_iterator iter = fields_tag_.begin();
             fields_tag_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->remove(key, n.to_set());
            }
        }
        return *this;
    }
    
    Storage &Storage::reindex(const unsigned long long key) {
        if (!key)
        {
            return *this;
        }
        
        Log::debug.log("Place [%d] in indicies.") << key << Log::end;
        BSONNode original = at(key);
        
        // Insert into tree index.
        for (std::map<std::string, TreeDB*>::const_iterator iter = fields_tree_.begin();
             fields_tree_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists() &&
                n.nested() &&
                _fields_unique.end() != _fields_unique.find(iter->first))
            {
                for (BSONNode::childmap_t::const_iterator iter2 = n.to_map().begin();
                     iter2 != n.to_map().end();
                     ++iter2)
                {
                    char* bson = iter2->second->bson();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->place_with_existing(bson + delta.first,
                                                      iter2->second->size() - delta.second,
                                                      &key,
                                                      sizeof(unsigned long long));
                    delete[] bson;
                }
            }
            else if (n.exists())
            {
                char* bson = n.bson();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->place_with_existing(bson + delta.first,
                                                  n.size() - delta.second,
                                                  &key,
                                                  sizeof(unsigned long long));
                delete[] bson;
            }
        }
        
        // Insert into hash index.
        for (std::map<std::string, Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists() &&
                n.nested() &&
                _fields_unique.end() != _fields_unique.find(iter->first))
            {
                for (BSONNode::childmap_t::const_iterator iter2 = n.to_map().begin();
                     iter2 != n.to_map().end();
                     ++iter2)
                {
                    char* bson = iter2->second->bson();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->place(bson + delta.first,
                                        iter2->second->size() - delta.second,
                                        &key,
                                        sizeof(unsigned long long));
                    delete[] bson;
                }
            }
            else if (n.exists())
            {
                char* bson = n.bson();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->place(bson + delta.first,
                                    n.size() - delta.second,
                                    &key,
                                    sizeof(unsigned long long));
                delete[] bson;
            }
        }
        
        // insert into text searcher
        for (std::map<std::string, TextSearcher *>::const_iterator iter = fields_text_.begin();
             fields_text_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->index(key, n.to_s());
            }
        }
        
        // insert into tag searcher
        for (std::map<std::string, TagSearcher *>::const_iterator iter = fields_tag_.begin();
             fields_tag_.end() != iter;
             ++iter)
        {
            BSONNode n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->index(key, n.to_set());
            }
        }
        return *this;
    }
    
    void Storage::begin_transaction() {
        db_->start_writes();
        for (std::map<std::string, TreeDB*>::const_iterator iter = fields_tree_.begin();
             fields_tree_.end() != iter;
             ++iter)
        {
            iter->second->start_writes();
        }
        for (std::map<std::string, Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            iter->second->start_writes();
        }
    }
    void Storage::commit_transaction() {
        for (std::map<std::string, Hash_db*>::reverse_iterator iter = fields_hash_.rbegin();
             fields_hash_.rend() != iter;
             ++iter)
        {
            iter->second->save_writes();
        }
        for (std::map<std::string, TreeDB *>::reverse_iterator iter = fields_tree_.rbegin();
             fields_tree_.rend() != iter;
             ++iter)
        {
            iter->second->save_writes();
        }
        db_->save_writes();
    }
    void Storage::abort_transaction()
    {
        for (std::map<std::string, Hash_db*>::reverse_iterator iter = fields_hash_.rbegin();
             fields_hash_.rend() != iter;
             ++iter)
        {
            iter->second->abort_writes();
        }
        for (std::map<std::string, TreeDB *>::reverse_iterator iter = fields_tree_.rbegin();
             fields_tree_.rend() != iter;
             ++iter)
        {
            iter->second->abort_writes();
        }
        db_->abort_writes();
    }
};