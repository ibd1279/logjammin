#pragma once
/*!
 \file Storage.h
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

#include "lj/Record_set.h"
#include "lj/Bson.h"
#include "tokyo/Tokyo.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace lj
{
    class Storage_factory;
    
    //! Storage Engine based on tokyo cabinet database libraries.
    /*!
     \par
     The storage engine represents a collection of indicies and a main document
     store.  When documents are placed into the storage engine, it indexes the
     document based upon a configuration.  Those configured indicies can be
     used to discover a document through the Record_set class.
     \par
     The Storage class loads configuration information from the DBDIR macro.
     The value of this macro can be set during build configuration:
     \code
     ./waf configure --dbdir=
     \endcode.
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     \sa lj::Record_set
     */
    class Storage {
        friend class Storage_factory;
        friend class Record_set;
    public:
        //! Destructor
        ~Storage();
        
        //! Get a single document Record_set.
        /*!
         \par
         This is a helper method for:
         \code
         none().include_key(key);
         \endcode.
         \param key The key to populate the Record_set with.
         \return A new Record_set object.
         */
        std::auto_ptr<Record_set> at(const unsigned long long key) const;
        
        //! Get a Record_set containing all keys.
        /*!
         \par
         The default operation for the all Record_set is \c lj::set::k_intersection.
         \return All keys.
         */
        std::auto_ptr<Record_set> all() const;
        
        //! Get an empty Record_set.
        /*!
         \par
         The default operation for the none Record_set is \c lj::set::k_union.
         \return No keys.
         */
        std::auto_ptr<Record_set> none() const;
        
        //! Store a document
        /*!
         \par
         Store a document.  If they document already has a key, it will
         replace the old document at that key.  If the document has a key of
         zero, it will be treated as a new document.
         \param value The document to place in storage.
         \return This storage object.
         */
        Storage& place(Bson &value);
        
        //! Remove a document
        /*!
         \par
         Document is deindexed and removed from the storage system.
         \param value The document to remove.
         \return This storage object.
         */
        Storage& remove(Bson &value);
        
        //! Begin a transaction.
        void begin_transaction();
        
        //! Commit a transaction.
        void commit_transaction();
        
        //! Rollback a transaction.
        void abort_transaction();
        
        Bson* configuration();
    protected:
        //! Open up a Storage engine.
        /*!
         \par
         Hidden. Use the Storage_factory instead.
         \sa lj::Storage_factory
         \param dir The document repository name.
         */
        Storage(const std::string &dir);
    private:
        //! Primary database.
        /*!
         \par Stores the actual documents, indexed by primary key.
         */
        tokyo::Tree_db* db_;
        
        //! Fields indexed using a tree db.
        std::map<std::string, tokyo::Tree_db*> fields_tree_;
        
        //! Fields indexed using a hash db.
        std::map<std::string, tokyo::Hash_db*> fields_hash_;
        
        //! Fields indexed using full text searcher.
        std::map<std::string, tokyo::TextSearcher*> fields_text_;
        
        //! Fields indexed using word searcher.
        std::map<std::string, tokyo::TagSearcher*> fields_tag_;
        
        //! Fields that have unique constraints.
        std::set<std::string> nested_indexing_;
        
        //! Directory where database files should be stored.
        std::string directory_;
        
        lj::Bson* config_;
        
        //! Remove a record from the indexed files.
        Storage &deindex(const unsigned long long key);
        
        //! Add a record to the indexed files.
        Storage &reindex(const unsigned long long key);
        
        //! Check that an existing record does not exist for a given value.
        Storage &check_unique(const Bson &n, const std::string &name, tokyo::DB *index);
        
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
