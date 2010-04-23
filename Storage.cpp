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

namespace tokyo {
    
    using logjam::Log;
    
    namespace {
        void dbvalue_to_storagekey(const DB::list_value_t &ptr,
                                   std::set<unsigned long long> &keys) {
            for(std::list<std::pair<void *, size_t> >::const_iterator iter = ptr.begin();
                iter != ptr.end();
                ++iter) {
                keys.insert(*((unsigned long long *)(iter->first)));
                free(iter->first);
            }
        }
    };
    
    //=====================================================================
    // Storage Filter Implementation.
    //=====================================================================
    
    DocumentNode StorageFilter::doc_at(unsigned long long pkey) const {
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
    
    StorageFilter &StorageFilter::filter(const std::string &indx,
                                        const void * const val,
                                        const size_t val_len) {
        std::map<std::string, TreeDB *>::const_iterator index = _storage->_fields_tree.find(indx);
        if(index == _storage->_fields_tree.end())
            return *this;
        
        DB *db = index->second;
        DB::list_value_t db_values;
        std::set<unsigned long long> storage_keys;
        db->at_together(val, val_len, db_values);
        dbvalue_to_storagekey(db_values, storage_keys);
        
        switch(_mode) {
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
        std::map<std::string, TextSearcher *>::const_iterator index = _storage->_fields_text.find(indx);
        if(index == _storage->_fields_text.end())
            return *this;
        
        TextSearcher *searcher = index->second;
        Searcher::set_key_t searcher_values;
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
        std::map<std::string, TagSearcher *>::const_iterator index = _storage->_fields_tag.find(indx);
        if(index == _storage->_fields_tag.end())
            return *this;
        
        TagSearcher *searcher = index->second;
        Searcher::set_key_t searcher_values;
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
    
    Storage::Storage(const std::string &dir) : _db(NULL), _fields_tree(), _fields_text(), _fields_tag(), _fields_unique(), _directory(DBDIR) {
        _directory.append("/").append(dir);
        std::string configfile(_directory + "/config");
        
        Log::info("Loading configuration from [%s].") << configfile << Log::end;
        DocumentNode cfg;
        cfg.load(configfile);
        Log::info("Loaded Settings [%s].") << cfg.to_pretty_s() << Log::end;
        
        std::string dbfile(_directory + "/" + cfg.nav("main/file").to_s());
        Log::info("Opening database [%s].") << dbfile << Log::end;
        _db = new TreeDB(dbfile,
                         BDBOREADER | BDBOWRITER | BDBOCREAT,
                         NULL);

        Log::info("Opening tree indices under [%s].") << _directory << Log::end;
        for(DocumentNode::childmap_t::const_iterator iter = cfg.nav("index/tree").to_map().begin();
            iter != cfg.nav("index/tree").to_map().end();
            ++iter) {
            if(!iter->second->nav("file").exists() || !iter->second->nav("field").exists()) {
                Log::error("Unable to open tree index [%s] because either file or field is not set.") << iter->first << Log::end; 
                continue;
            }
            std::string indexfile(_directory + "/" + iter->second->nav("file").to_s());
            Log::info("Opening tree index [%s].") << indexfile << Log::end;
            TreeDB *tdb = new TreeDB(indexfile,
                                     BDBOREADER | BDBOWRITER | BDBOCREAT,
                                     NULL);
            _fields_tree.insert(std::pair<std::string, TreeDB *>(iter->second->nav("field").to_s(), tdb));
        }
        
        Log::info("Opening text indices under [%s].") << _directory << Log::end;
        for(DocumentNode::childmap_t::const_iterator iter = cfg.nav("index/text").to_map().begin();
            iter != cfg.nav("index/text").to_map().end();
            ++iter) {
            if(!iter->second->nav("file").exists() || !iter->second->nav("field").exists()) {
                Log::error("Unable to open text index [%s] because either file or field is not set.") << iter->first << Log::end;
                continue;
            }
            std::string indexfile(_directory + "/" + iter->second->nav("file").to_s());
            Log::info("Opening text index [%s].") << indexfile << Log::end;
            TextSearcher *ts = new TextSearcher(indexfile,
                                                QDBOREADER | QDBOWRITER | QDBOCREAT,
                                                NULL);
            _fields_text.insert(std::pair<std::string, TextSearcher *>(iter->second->nav("field").to_s(), ts));
        }

        logjam::Log::info("Opening tag indices under [%s].") << _directory << Log::end;
        for(DocumentNode::childmap_t::const_iterator iter = cfg.nav("index/tag").to_map().begin();
            iter != cfg.nav("index/tag").to_map().end();
            ++iter) {
            if(!iter->second->nav("file").exists() || !iter->second->nav("field").exists()) {
                Log::error("Unable to open tag index [%s] because either file or field is not set.") << iter->first << Log::end;
                continue;
            }
            std::string indexfile(_directory + "/" + iter->second->nav("file").to_s());
            Log::info("Opening tag index [%s].") << indexfile << Log::end;
            TagSearcher *ts = new TagSearcher(indexfile,
                                              WDBOREADER | WDBOWRITER | WDBOCREAT,
                                              NULL);
            _fields_tag.insert(std::pair<std::string, TagSearcher *>(iter->second->nav("field").to_s(), ts));
        }
    }
    
    Storage::~Storage() {
        Log::info("Closing tag indicies under [%s].") << _directory << Log::end;
        for(std::map<std::string, TagSearcher *>::const_iterator iter = _fields_tag.begin();
            iter != _fields_tag.end();
            ++iter) {
            Log::info("Closing tag index for field [%s].") << iter->first << Log::end;
            delete iter->second;
        }
        Log::info("Closing database for field [%s].") << _directory << Log::end;
        for(std::map<std::string, TextSearcher *>::const_iterator iter = _fields_text.begin();
            iter != _fields_text.end();
            ++iter) {
            Log::info("Closing text index for field [%s].") << iter->first << Log::end;
            delete iter->second;
        }
        Log::info("Closing database for field [%s].") << _directory << Log::end;
        for(std::map<std::string, TreeDB *>::const_iterator iter = _fields_tree.begin();
            iter != _fields_tree.end();
            ++iter) {
            Log::info("Closing tree index for field [%s].") << iter->first << Log::end;
            delete iter->second;
        }
        Log::info("Closing database for field [%s].") << _directory << Log::end;
        delete _db;
    }
    
    DocumentNode Storage::at(const unsigned long long key) const {
        DB::value_t p = _db->at(&key, sizeof(unsigned long long));
        if(!p.first)
            return DocumentNode();
        DocumentNode n(DOC_NODE, (char *)p.first);
        free(p.first);
        return n;
    }
    
    StorageFilter Storage::all() const {
        DB::list_value_t keys;
        DB::value_t max = _db->max_key(), min = _db->min_key();
        if(_db->range_keys(min.first,
                           min.second,
                           true,
                           max.first,
                           max.second,
                           true,
                           keys)) {
            std::set<unsigned long long> real_keys;
            dbvalue_to_storagekey(keys, real_keys);
            return StorageFilter(this, real_keys);
        }
        return StorageFilter(this, std::set<unsigned long long>());
    }
    
    StorageFilter Storage::none() const {
        return StorageFilter(this, std::set<unsigned long long>());
    }
    
    StorageFilter Storage::filter(const std::string &indx,
                                  const void * const val,
                                  const size_t val_len) const {
        std::map<std::string, TreeDB *>::const_iterator index = _fields_tree.find(indx);
        if(index == _fields_tree.end())
            return none();
        
        DB *db = index->second;
        DB::list_value_t db_values;
        std::set<unsigned long long> storage_keys;
        db->at_together(val, val_len, db_values);
        dbvalue_to_storagekey(db_values, storage_keys);
        
        return StorageFilter(this, storage_keys);
    }
    
    StorageFilter Storage::search(const std::string &indx,
                                        const std::string &terms) const {
        std::map<std::string, TextSearcher *>::const_iterator index = _fields_text.find(indx);
        if(index == _fields_text.end())
            return none();
        
        TextSearcher *searcher = index->second;
        Searcher::set_key_t searcher_values;
        searcher->search(terms, searcher_values);
        
        return StorageFilter(this, searcher_values);
    }
    
    StorageFilter Storage::tagged(const std::string &indx,
                                        const std::string &word) const {
        std::map<std::string, TagSearcher *>::const_iterator index = _fields_tag.find(indx);
        if(index == _fields_tag.end())
            return none();
        
        TagSearcher *searcher = index->second;
        Searcher::set_key_t searcher_values;
        searcher->search(word, searcher_values);
        
        return StorageFilter(this, searcher_values);
    }
    
    Storage &Storage::place(DocumentNode &value) {
        unsigned long long key = value.nav("__key").to_l();
        unsigned long long original_key = value.nav("__key").to_l();
        try {
            begin_transaction();
            if(key) {
                deindex(key);
            } else {
                // calculate the next key since this is a new document.
                unsigned long long *ptr = (unsigned long long *)_db->max_key().first;
                key = (*ptr) + 1;
            }
            
            // Enforce unique constraints.
            for(std::set<std::string>::const_iterator iter = _fields_unique.begin();
                iter != _fields_unique.end();
                ++iter) {
                DocumentNode n(value.nav(*iter));
                if(n.exists()) {
                    std::map<std::string, TreeDB *>::const_iterator index = _fields_tree.find(*iter);
                    if(index != _fields_tree.end()) {
                        char *bson = n.bson();
                        DB::value_t existing = index->second->at(bson,
                                                                 n.size());
                        delete[] bson;
                        if(existing.first)
                            throw Exception("StorageError",
                                            std::string("Unable to place record because of unique constraint [").append(*iter).append("]."));
                    }
                }
            }
            
            // Place in the primary database.
            value.nav("__key").value((long long)key);
            char *bson = value.bson();
            _db->place(&key,
                       sizeof(unsigned long long),
                       bson,
                       value.size());
            delete[] bson;
            
            reindex(key);
            commit_transaction();
        } catch(Exception ex) {
            value.nav("__key").value((long long)original_key);
            abort_transaction();
            throw ex;
        }
        return *this;
    }
    
    Storage &Storage::remove(DocumentNode &value) {
        unsigned long long key = value.nav("__key").to_l();
        if(key) {
            try {
                begin_transaction();
                deindex(key);
                _db->remove(&key,
                            sizeof(unsigned long long));
                commit_transaction();
                value.nav("__key").value(0LL);
            } catch(Exception ex) {
                abort_transaction();
                throw ex;
            }
        }
        return *this;
    }
    
    Storage &Storage::deindex(const unsigned long long key) {
        if(!key) return *this;
        
        // Get the original document
        DocumentNode original = at(key);
        
        // Remove from index entries.
        for(std::map<std::string, TreeDB *>::const_iterator iter = _fields_tree.begin();
            iter != _fields_tree.end();
            ++iter) {
            DocumentNode n(original.nav(iter->first));
            if(n.exists()) {
                char *bson = n.bson();
                iter->second->remove_from_existing(bson,
                                                   n.size(),
                                                   &key,
                                                   sizeof(unsigned long long));
                delete[] bson;
            }
        }
        for(std::map<std::string, TextSearcher *>::const_iterator iter = _fields_text.begin();
            iter != _fields_text.end();
            ++iter) {
            DocumentNode n(original.nav(iter->first));
            if(n.exists()) {
                iter->second->remove(key, n.to_s());
            }
        }
        for(std::map<std::string, TagSearcher *>::const_iterator iter = _fields_tag.begin();
            iter != _fields_tag.end();
            ++iter) {
            DocumentNode n(original.nav(iter->first));
            if(n.exists()) {
                iter->second->remove(key, n.to_set());
            }
        }
        return *this;
    }
    Storage &Storage::reindex(const unsigned long long key) {
        if(!key) return *this;
        
        // Get the original document
        DocumentNode original = at(key);
        
        // Place in the index.
        for(std::map<std::string, TreeDB *>::const_iterator iter = _fields_tree.begin();
            iter != _fields_tree.end();
            ++iter) {
            DocumentNode n(original.nav(iter->first));
            if(n.exists()) {
                char *bson = n.bson();
                if(_fields_unique.find(iter->first) == _fields_unique.end()) {
                    iter->second->place_with_existing(bson,
                                                      n.size(),
                                                      &key,
                                                      sizeof(unsigned long long));
                } else {
                    iter->second->place(bson,
                                        n.size(),
                                        &key,
                                        sizeof(unsigned long long));
                }
                delete[] bson;
            }
        }
        for(std::map<std::string, TextSearcher *>::const_iterator iter = _fields_text.begin();
            iter != _fields_text.end();
            ++iter) {
            DocumentNode n(original.nav(iter->first));
            if(n.exists()) {
                iter->second->index(key, n.to_s());
            }
        }
        for(std::map<std::string, TagSearcher *>::const_iterator iter = _fields_tag.begin();
            iter != _fields_tag.end();
            ++iter) {
            DocumentNode n(original.nav(iter->first));
            if(n.exists()) {
                iter->second->index(key, n.to_set());
            }
        }
        return *this;
    }
    void Storage::begin_transaction() {
        _db->start_writes();
        for(std::map<std::string, TreeDB *>::const_iterator iter = _fields_tree.begin();
            iter != _fields_tree.end();
            ++iter) {
            iter->second->start_writes();
        }
    }
    void Storage::commit_transaction() {
        for(std::map<std::string, TreeDB *>::reverse_iterator iter = _fields_tree.rbegin();
            iter != _fields_tree.rend();
            ++iter) {
            iter->second->save_writes();
        }
        _db->save_writes();
    }
    void Storage::abort_transaction() {
        for(std::map<std::string, TreeDB *>::reverse_iterator iter = _fields_tree.rbegin();
            iter != _fields_tree.rend();
            ++iter) {
            iter->second->abort_writes();
        }
        _db->abort_writes();
    }
};