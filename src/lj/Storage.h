#pragma once
/*!
 \file lj/Storage.h
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
    class Vault;
    class Index;
    
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
     ./waf configure
     \endcode.
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     \sa lj::Record_set
     */
    class Storage
    {
        friend class Storage_factory;
        friend class Record_set;
    public:
        
        //! Removed
        /*!
         \param o Copy from object.
         */
        Storage(const Storage& o) = delete;
        
        //! Removed
        /*!
         \param o Copy from object.
         */
        Storage& operator=(const Storage& o) = delete;

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
        std::unique_ptr<Record_set> at(const unsigned long long key) const;
        
        //! Get a Record_set containing all keys.
        /*!
         \par
         The default operation for the all Record_set is \c lj::set::k_intersection.
         \return All keys.
         */
        std::unique_ptr<Record_set> all() const;

        const lj::Index* const index(const std::string& indx) const
        {
            return NULL;
        }

        const lj::Vault* const vault() const
        {
            return NULL;
        }
        
        //! Get an empty Record_set.
        /*!
         \par
         The default operation for the none Record_set is \c lj::set::k_union.
         \return No keys.
         */
        std::unique_ptr<Record_set> none() const;
        
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
        
        //! Get the configuration.
        /*!
         \return Bson object.
         */
        Bson* configuration();
        
        //! Get the name associated with the loading of this storage object.
        /*!
         \return the name.
         */
        const std::string& name() const;
        
        //! Checkpoint the database.
        /*!
         \par
         Checkpointing the database causes the journal to be cleared and a copy
         of the database is created.
         */
        void checkpoint();
        
        //! Delete all index files and rebuild.
        void rebuild();
        
        //! Rebuild a specific index.
        void rebuild_field_index(const std::string& index);
        
        //! Optimize all the indices.
        void optimize();
    protected:
        //! Open up a Storage engine.
        /*!
         \par
         Hidden. Use the Storage_factory instead.
         \sa lj::Storage_factory
         \param dir The document repository name.
         \param server_config The configuration for the storage server.
         */
        Storage(const std::string &dir,
                const lj::Bson& server_config);
        
    private:
        //! Primary database.
        /*!
         \par Stores the actual documents, indexed by primary key.
         */
        tokyo::Tree_db* db_;
        
        //! Journal database.
        /*!
         \par Journal used to track database actions for recovery purposes.
         */
        tokyo::Fixed_db* journal_;
        
        //! Fields indexed using a tree db.
        std::map<std::string, tokyo::Tree_db*> fields_tree_;
        
        //! Fields indexed using a hash db.
        std::map<std::string, tokyo::Hash_db*> fields_hash_;
        
        //! Fields that have unique constraints.
        std::set<std::string> nested_indexing_;
        
        //! Configuration.
        lj::Bson* config_;
        
        //! Server configuration;
        const lj::Bson& server_config_;
        
        //! name of the storage object.
        std::string name_;
        
        //! Remove a record from the indexed files.
        Storage &deindex(const lj::Bson& record);
        
        //! Add a record to the indexed files.
        Storage &reindex(const lj::Bson& record);
        
        //! Check that an existing record does not exist for a given value.
        Storage &check_unique(const Bson &n,
                              const std::string &name,
                              tokyo::DB *index);
        
        //! Write a journal entry at the start of a modification action.
        void journal_start(const unsigned long long key);
        
        //! Update the journal entry to completed.
        void journal_end(const unsigned long long key);
    };
    
    //! Create an empty basic storage configuration.
    /*!
     \par
     This is used to create a default new storage configuration with no
     options.
     \param cfg The configuration to populate the values into.
     \param name The name to use for the new storage.
     */
    void storage_config_init(lj::Bson& cfg,
                             const std::string& name);

    //! Add an index to a storage configuration.
    /*!
     \par
     Create an indexed attribute on the storage configuration.
     \param cfg The storage configuration to modify.
     \param type The type of index to build. [hash, tree, tag, text]
     \param field The field to index in placed documents.
     \param comp The type of comparison to use.
     */
    void storage_config_add_index(lj::Bson& cfg,
                                  const std::string& type,
                                  const std::string& field,
                                  const std::string& comp);

    //! Remove an index from a storage configuration.
    /*!
     \par
     Remove an indexed attribute on the storage configuration.
     \param cfg The storage configuration to modify.
     \param type The type of index to remove.
     \param field The field used to create the index.
     */
    void storage_config_remove_index(lj::Bson& cfg,
                                     const std::string& type,
                                     const std::string& field);

    //! Classify a field as containing multiple values.
    /*!
     \par
     Marks a document field as containing multiple elements.
     \param cfg The storage configuration to modify.
     \param field The field.
     */
    void storage_config_add_subfield(lj::Bson& cfg,
                                     const std::string& field);
    //! Save a storage configuration to disk.
    /*!
     \par
     Write a storage configuration to disk. Saves the file where
     the server configuration points for the data directory.
     \param cfg The storage configuration to save.
     \param server_config The configuration of the server.
     */
    void storage_config_save(const lj::Bson& cfg,
                             const lj::Bson& server_config);
    //! Load a storage configuration from disk.
    /*!
     \par
     Load a storage configuration from disk. The file is loaded
     from where the server configuration points for the data
     directory.
     \param dbname The name of the storage.
     \param server_config The configuration of the server.
     \return Pointer (allocated with new) to the storage configuration.
     */
    lj::Bson* storage_config_load(const std::string& dbname,
                                  const lj::Bson& server_config);
    
}; // namespace lj
