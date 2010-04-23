#pragma once
/*
 \file Storage.h
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

#include "Tokyo.h"
#include "DocumentNode.h"
#include <list>
#include <map>
#include <set>
#include <string>

namespace tokyo {
    class Storage;
    
    //! Filter used in Storage Queries.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class StorageFilter {
    public:
        //! Storage Filter Mode.
        /*!
         \par
         Used to control how the filter evaluates key sets.
         */
        enum StorageFilterMode {
            //! Similar to an "AND".
            INTERSECTION,
            
            //! Similar to an "OR".
            UNION
        };
    protected:
        //! Storage pointer.  All filtering is done with this storage engine.
        const Storage *_storage;
        
        //! Set of keys represented by this filter.
        std::set<unsigned long long> _keys;
        
        //! Mode for performing key evaluations.
        StorageFilterMode _mode;
        
        //! XXX Not Yet Used - Where to start returning results.
        long long _offset;
        
        //! XXX Not Yet Used - How many results to return.
        long long _length;
        
        //! Internal method to hide some logic for the template code below.
        DocumentNode doc_at(unsigned long long pkey) const;
    public:
        //! Create a new Storage Filter.
        StorageFilter(const Storage *storage,
                      const std::set<unsigned long long> &keys,
                      StorageFilterMode mode = INTERSECTION,
                      long long offset = -1,
                      long long length = -1);
        
        //! Create a new StorageFilter as a copy of an existing StorageFilter.
        StorageFilter(const StorageFilter &orig);
        
        //! Destructor.
        ~StorageFilter();
        
        //! Set the storage filtering mode.  Default mode is INTERSECTION.
        StorageFilter &mode(const StorageFilterMode mode) { _mode = mode; return *this; };
        
        //! Check if the filtered set contains a key.
        bool contains(unsigned long long key) const;
        
        //! Add keys to the filtered set.
        StorageFilter &union_keys(const std::set<unsigned long long> &keys);
        
        //! Add a key to the filtered set.
        StorageFilter &union_key(unsigned long long key) {
            std::set<unsigned long long> tmp;
            tmp.insert(key);
            return union_keys(tmp);
        }
        
        //! Eliminate filtered set keys.
        StorageFilter &intersect_keys(const std::set<unsigned long long> &keys);
        
        //! Reduce filtered set to a 1 or 0 keys.
        StorageFilter &intersect_key(unsigned long long key) {
            std::set<unsigned long long> tmp;
            tmp.insert(key);
            return intersect_keys(tmp);
        }
        
        //! Filter a set based on the Storage field indicies.
        StorageFilter &filter(const std::string &indx,
                             const void * const val,
                             const size_t val_len);
        
        //! Filter a set based on the Storage text indicies.
        StorageFilter &search(const std::string &indx,
                             const std::string &terms);
        
        //! Filter a set based on the Storage word indicies.
        StorageFilter &tagged(const std::string &indx,
                             const std::string &word);
        
        //! How many keys are in the set.
        unsigned long long size() { return _keys.size(); }
        
        //! Get the list of items contained by the filter.
        template<typename D>
        bool items(std::list<D *> &results,
                   const long long start = -1,
                   const long long end = -1) const {
            for(std::set<unsigned long long>::const_iterator iter = _keys.begin();
                iter != _keys.end();
                ++iter) {
                results.push_back(new D(doc_at(*iter)));
            }
            return true;
        }
    };
    
    //! Storage Engine based on tokyo cabinet database libraries.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class Storage {
        friend class StorageFilter;
    protected:
        //! Primary database.
        /*!
         \par Stores the actual documents, indexed by primary key.
         */
        TreeDB *_db;
        
        //! Fields using a tree index.
        /*!
         \par used for range selected values.
         */
        std::map<std::string, TreeDB *> _fields_tree;
        
        //! Fields indexed using full text searcher.
        std::map<std::string, TextSearcher *> _fields_text;
        
        //! Fields indexed using word searcher.
        std::map<std::string, TagSearcher *> _fields_tag;
        
        //! Fields that have unique constraints.
        std::set<std::string> _fields_unique;
        
        //! Directory where database files should be stored.
        std::string _directory;
        
        //! Remove a record from the indexed files.
        virtual Storage &deindex(const unsigned long long key);
        
        //! Add a record to the indexed files.
        virtual Storage &reindex(const unsigned long long key);
    public:
        //! Consructor
        Storage(const std::string &dir);
        
        //! Destructor
        virtual ~Storage();
        
        //! Get the document stored for the key.
        virtual DocumentNode at(const unsigned long long key) const;
        
        //! Get a set of all keys.
        virtual StorageFilter all() const;
        
        //! Get a set of no keys.
        virtual StorageFilter none() const;
        
        //! Get a set of keys filtered by the provided value.
        virtual StorageFilter filter(const std::string &indx,
                                     const void * const val,
                                     const size_t val_len) const;
        
        //! Get a set of keys filtered by the full text search.
        virtual StorageFilter search(const std::string &indx,
                                     const std::string &terms) const;
        
        //! Get a set of keys filtered by the word search.
        virtual StorageFilter tagged(const std::string &indx,
                                     const std::string &word) const;
        
        //! place a document in storage.
        virtual Storage &place(DocumentNode &value);
        
        //! remove a document from storage.
        virtual Storage &remove(DocumentNode &value);
        
        //! Begin a transaction.
        void begin_transaction();
        
        //! Commit a transaction.
        void commit_transaction();
        
        //! Rollback a transaction.
        void abort_transaction();
    };
}; // namespace tokyo