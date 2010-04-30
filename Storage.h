#pragma once
/*
 \file Storage.h
 \brief Storage and StorageFilter header.
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
#include "BSONNode.h"
#include <list>
#include <map>
#include <set>
#include <string>

namespace lj
{
    class Storage;
    
    /*!
     \namespace lj::operation
     \brief Contains enumeration for Set operations.
     */
    namespace operation
    {
        //! Set operation.
        enum Set
        {
            k_intersection,        //!< Similar to an "AND".
            k_union,               //!< Similar to an "OR".
            k_complement,          //!< Similar to a "NOT".
            k_symmetric_difference //!< Similar to a "XOR"
        };
    }; // operation
    
    //! Perform an operation on two sets.
    /*!
     \par
     The provided \c op is performed on the two sets and the result is returned
     as a pointer to a third set. The operation is performed such that \c a
     \c op \c b . So keep in mind the commutative properties of the operation
     you are performing.
     \param op The operation to perform.
     \param a The first set.
     \param b The second set.
     \return A newly created set.
     */
    template<typename T, typename Q>
    const T *set_operation(const operation::Set op,
                               const T& a,
                               const T& b)
    {
        const T* small = (a.size() < b.size()) ? &a : &b;
        const T* big = (a.size() < b.size()) ? &b : &a;
        T* rs = new std::set<T>();
        Q inserted_at = rs->begin();
        switch (op)
        {
            case operation::k_intersection:
                for (Q iter = small->begin();
                     small->end() != iter;
                     ++iter)
                {
                    if (big->end() != big->find(*iter))
                    {
                        inserted_at = rs->insert(inserted_at, *iter);
                    }
                }
                break;
            case operation::k_union:
                rs->insert(big->begin(), big->end());
                rs->insert(small->begin(), small->end());
                break;
            case operation::k_symmetric_difference:
                for (Q iter = b.begin();
                     b.end() != iter;
                     ++iter)
                {
                    if (a.end() == a.find(*iter))
                    {
                        inserted_at = rs->insert(inserted_at, *iter);
                    }
                }
                inserted_at = rs->begin();
                // fall through.
            case operation::k_complement:
                for (Q iter = a.begin();
                     a.end() != iter;
                     ++iter)
                {
                    if (b.end() == b.find(*iter))
                    {
                        inserted_at = rs->insert(inserted_at, *iter);
                    }
                }
                break;
        }
        return rs;
    }
        
    //! Filter used in Storage queries.
    /*!
     \par
     The storage filter is really an abstract representation of a set of
     documents. The refine, search, and tagged methods are used to modify
     the set of documents to match a certain set of criteria. The following
     examples shows a usage to get a list of all users with the first name
     "Jason" and the last name "Watson".
     \code
     Storage storage("user");
     std::list<BSONNode> records;
     storage.refine("first_name", "Jason").refine("last_name", "Watson").items<BSONNode>(records);
     \endcode
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class StorageFilter {
    public:        
        typedef std::set<unsigned long long> Set_type;
        
        //! Perform an operation between two Record_set objects.
        /*!
         \par
         Perform the provided \c op on the two provided sets and return a third
         set representing the result. This method is used to perform set
         algebration operations against the different Record_set objects.
         \param op The operation to perform.
         \param a The first set.
         \param b The second set.
         \return a new set containing the operation result.
         */
        static StorageFilter operate(const operation::Set op,
                                     const StorageFilter& a,
                                     const StorageFilter& b);
        
        //! Create a new Storage Filter.
        StorageFilter(const Storage *storage,
                      const std::set<unsigned long long> &keys,
                      operation::Set mode,
                      long long offset = -1,
                      long long length = -1);
        
        //! Create a new StorageFilter as a copy of an existing StorageFilter.
        StorageFilter(const StorageFilter &orig);
        
        //! Destructor.
        ~StorageFilter();
        
        //! Set the storage filtering mode.  Default mode is INTERSECTION.
        StorageFilter &mode(const operation::Set mode) { _mode = mode; return *this; };
        
        //! Check if the filtered set contains a key.
        bool contains(unsigned long long key) const;
        
        //! Add keys to the filtered set.
        StorageFilter &union_keys(const std::set<unsigned long long> &keys);
        
        //! Add a key to the filtered set.
        StorageFilter &union_key(unsigned long long key)
        {
            std::set<unsigned long long> tmp;
            tmp.insert(key);
            return union_keys(tmp);
        }
        
        //! Eliminate filtered set keys.
        StorageFilter &intersect_keys(const std::set<unsigned long long> &keys);
        
        //! Reduce filtered set to a 1 or 0 keys.
        StorageFilter &intersect_key(unsigned long long key)
        {
            std::set<unsigned long long> tmp;
            tmp.insert(key);
            return intersect_keys(tmp);
        }
        
        //! Filter a set based on the Storage field indicies.
        StorageFilter &refine(const std::string &indx,
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
        bool items(std::list<D> &results,
                   const long long start = -1,
                   const long long end = -1) const {
            for(std::set<unsigned long long>::const_iterator iter = _keys.begin();
                iter != _keys.end();
                ++iter) {
                D obj;
                obj.assign(doc_at(*iter));
                results.push_back(obj);
            }
            return true;
        }
        
        template<typename D>
        bool items(std::list<D *> &results,
                   const long long start = -1,
                   const long long end = -1) const {
            for(std::set<unsigned long long>::const_iterator iter = _keys.begin();
                iter != _keys.end();
                ++iter) {
                D *obj = new D();
                obj->assign(doc_at(*iter));
                results.push_back(obj);
            }
            return true;
        }
        
        template<typename D>
        bool first(D &result) const {
            for(std::set<unsigned long long>::const_iterator iter = _keys.begin();
                iter != _keys.end();
                ++iter) {
                result.assign(doc_at(*iter));
                return true;
            }
            return false;
        }
    private:
        //! Storage pointer.  All filtering is done with this storage engine.
        const Storage *_storage;
        
        //! Set of keys represented by this filter.
        std::set<unsigned long long> _keys;
        
        //! Mode for performing key evaluations.
        operation::Set _mode;
        
        //! XXX Not Yet Used - Where to start returning results.
        long long _offset;
        
        //! XXX Not Yet Used - How many results to return.
        long long _length;
        
        //! Internal method to hide some logic for the template code below.
        BSONNode doc_at(unsigned long long pkey) const;
    };
    
    //! Storage Engine based on tokyo cabinet database libraries.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class Storage {
        friend class StorageFilter;
    public:
        //! Consructor
        Storage(const std::string &dir);
        
        //! Destructor
        virtual ~Storage();
        
        //! Get the document stored for the key.
        virtual BSONNode at(const unsigned long long key) const;
        
        //! Get a set of all keys.
        virtual StorageFilter all() const;
        
        //! Get a set of no keys.
        virtual StorageFilter none() const;
        
        //! Get a set of keys filtered by the provided value.
        virtual StorageFilter refine(const std::string &indx,
                                     const void * const val,
                                     const size_t val_len) const;
        
        //! Get a set of keys filtered by the full text search.
        virtual StorageFilter search(const std::string &indx,
                                     const std::string &terms) const;
        
        //! Get a set of keys filtered by the word search.
        virtual StorageFilter tagged(const std::string &indx,
                                     const std::string &word) const;
        
        //! place a document in storage.
        virtual Storage &place(BSONNode &value);
        
        //! remove a document from storage.
        virtual Storage &remove(BSONNode &value);
        
        //! Begin a transaction.
        void begin_transaction();
        
        //! Commit a transaction.
        void commit_transaction();
        
        //! Rollback a transaction.
        void abort_transaction();
    private:
        //! Primary database.
        /*!
         \par Stores the actual documents, indexed by primary key.
         */
        tokyo::TreeDB *db_;
        
        //! Fields indexed using a tree db.
        std::map<std::string, tokyo::TreeDB *> fields_tree_;
        
        //! Fields indexed using a hash db.
        std::map<std::string, tokyo::Hash_db*> fields_hash_;
        
        //! Fields indexed using full text searcher.
        std::map<std::string, tokyo::TextSearcher *> fields_text_;
        
        //! Fields indexed using word searcher.
        std::map<std::string, tokyo::TagSearcher *> fields_tag_;
        
        //! Fields that have unique constraints.
        std::set<std::string> nested_indexing_;
        
        //! Directory where database files should be stored.
        std::string directory_;
        
        //! Remove a record from the indexed files.
        virtual Storage &deindex(const unsigned long long key);
        
        //! Add a record to the indexed files.
        virtual Storage &reindex(const unsigned long long key);
        
        //! Check that an existing record does not exist for a given value.
        virtual Storage &check_unique(const BSONNode &n, const std::string &name, tokyo::DB *index);
        
        //! Hidden copy constructor
        /*!
         \param o Copy from object.
         */
        Storage(const Storage& o);
        
        //! Hidden assignment operator.
        /*!
         \param o Copy from object.
         */
        Storage& operator=(const Storage& o);
    };
}; // namespace lj
