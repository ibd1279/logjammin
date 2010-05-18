#pragma once
/*
 \file Storage.h
 \brief Storage and Record_set header.
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
#include "Bson.h"
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace lj
{
    class Storage;
    
    /*!
     \namespace lj::set
     \brief Contains enumeration for Set operations.
     */
    namespace set
    {
        //! Set operation.
        enum Operation
        {
            k_intersection,        //!< Similar to an "AND".
            k_union,               //!< Similar to an "OR".
            k_complement,          //!< Similar to a "NOT".
            k_symmetric_difference //!< Similar to a "XOR"
        };
    }; // set
    
    
    //! Abstract collection of documents.
    /*!
     \par
     The Record_set is really an abstract representation of a set of
     documents. The refine, search, and tagged methods are used to modify
     the set of documents to match a certain set of criteria. The following
     examples shows a usage to get a list of all users with the first name
     "Jason" and the last name "Watson".
     \code
     Storage storage("user");
     std::list<BSONNode> records;
     storage.equal("first_name", "Jason").equal("last_name", "Watson").items<BSONNode>(records);
     \endcode
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     \sa lj::Storage
     */
    class Record_set {
    public:
        //! Constructor.
        Record_set()
        {
        }
        
        //! Destructor.
        virtual ~Record_set()
        {
        }
        
        //! Set the Record_set operation.
        /*!
         \par
         The operation provided is used for creating the response object from
         the refine, search, and tagged methods.
         \param op The new default operation to perform.
         \return The modified Record_set object.
         */
        virtual Record_set& set_operation(const set::Operation op) = 0;
        
        //! Check if the Record_set contains a key.
        /*!
         \param key The key to test for.
         \return True if the key is included. False otherwise.
         */
        virtual bool is_included(const unsigned long long key) const = 0;
        
        //! Add keys to the Record_set.
        /*!
         \par
         Keys are copied from the input set.
         \param keys The keys to add.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> include_keys(const std::set<unsigned long long>& keys) = 0;
        
        //! Add a key to the Record_set.
        /*!
         \param key The key to add.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> include_key(const unsigned long long key) = 0;
        
        //! Remove keys from the Record_set.
        /*!
         \param keys The keys to exclude.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> exclude_keys(const std::set<unsigned long long> &keys) = 0;
        
        //! Remove a key from the Record_set.
        /*!
         \param key The key to remove.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> exclude_key(const unsigned long long key) = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for the value \c val. The results from the index are
         operated with the current Record_set.
         \par
         If a Hash_db exists for \c indx, it will be used.  If it does not exist,
         a Tree_db will be used. If neither index exists, a copy of the current
         Record_set is returned.
         \param indx The index to search against.
         \param val The value to search for.
         \param len The length of the value.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> equal(const std::string& indx,
                                           const void* const val,
                                           const size_t len) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values greater than \c val.
         The results from the index are operated with the current Record_set.
         \par
         If a Tree_db exists for \c indx, it will be used. If an index does
         not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param val The value to search for.
         \param len The length of the value.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> greater(const std::string& indx,
                                             const void* const val,
                                             const size_t len) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values lesser than \c val.
         The results from the index are operated with the current Record_set.
         \par
         If a Tree_db exists for \c indx, it will be used. If an index does
         not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param val The value to search for.
         \param len The length of the value.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> lesser(const std::string& indx,
                                            const void* const val,
                                            const size_t len) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values containing \c term.
         The results from the index are operated with the current Record_set.
         \par
         If a Text_searcher exists for \c indx, it will be used. If an index
         does not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param term The value to search for.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> contains(const std::string& indx,
                                              const std::string& term) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values containing \c word.
         The results from the index are operated with the current Record_set.
         \par
         If a Word_searcher exists for \c indx, it will be used. If an index
         does not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param word The value to search for.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> tagged(const std::string& indx,
                                            const std::string& word) const = 0;
        
        //! Record_set size.
        /*!
         \return The number of documents currently in the set.
         */
        virtual unsigned long long size() const = 0;
        
        //! Get the documents in this Record_set.
        /*!
         \par
         Get a list of items, as type \c D and return them in \c records.
         \param records The list to place items into.
         \return True when records have been added.
         */
        virtual bool items(std::list<Bson>& records) const = 0;
        
        //! Get the documents in this Record_set as pointers.
        /*!
         \par
         Get a list of items, as type \c D* and return them in \c records.
         \param records The list to place items into.
         \return True when records have been added.
         */
        virtual bool items(std::list<Bson*>& records) const = 0;
        
        //! Get the first document in the Record_set.
        /*!
         \par
         Because the STL set backing the Record_set is ordered, the first
         document will always be the lowest key document in the set.
         \param record Record to populate.
         \return True when the record has been modified.
         */
        virtual bool first(Bson& result) const = 0;
    protected:
        static tokyo::TreeDB* storage_db(const Storage* s);
        static tokyo::TreeDB* storage_tree(const Storage* s, const std::string& indx);
        static tokyo::Hash_db* storage_hash(const Storage* s, const std::string& indx);
        static tokyo::TextSearcher* storage_text(const Storage* s, const std::string& indx);
        static tokyo::TagSearcher* storage_tag(const Storage* s, const std::string& indx);
    };
    
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
        friend class Record_set;
    public:
        //! Open up a Storage engine.
        /*!
         \par
         The settings information for this storage engine is loaded from
         \c DBDIR \c + \c "/" \c + \c dir. The settings file can be created by
         executing the logjam shell command. The following is an example of a
         storage engine configuration:
         \code
         role_cfg = sc_new("role")
         sc_add_index(role_cfg, "hash", "name", "name", "lex")
         sc_add_index(role_cfg, "tree", "allowed", "allowed", "lex")
         sc_add_index(role_cfg, "text", "allowed", "allowed", "lex")
         sc_add_index(role_cfg, "text", "name", "name", "lex")
         sc_add_index(role_cfg, "tag", "allowed", "allowed", "lex")
         sc_add_index(role_cfg, "tag", "name", "name", "lex")
         sc_add_unique(role_cfg, "allowed")
         sc_save("role", role_cfg)         
         \endcode
         \param dir The document repository name.
         */
        Storage(const std::string &dir);
        
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
         This method may pose a memory problem on large document stores.
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
    private:
        //! Primary database.
        /*!
         \par Stores the actual documents, indexed by primary key.
         */
        tokyo::TreeDB* db_;
        
        //! Fields indexed using a tree db.
        std::map<std::string, tokyo::TreeDB*> fields_tree_;
        
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
