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

#include "lj/Bson.h"
#include "lj/Engine.h"

#include <map>
#include <memory>
#include <string>

namespace lj
{
    class Storage_factory;
    
    //! Storage Interface
    /*!
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     \sa lj::Record_set
     */
    class Storage
    {
        friend class Storage_factory;
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
        
        const lj::Index* const index(const std::string& indx) const;

        const lj::Vault* const vault() const;
        
        //! Store a document
        Storage& place(Bson &value);
        
        //! Remove a document
        Storage& remove(Bson &value);
        
        //! Get the configuration.
        /*!
         \return Bson object.
         */
        Bson* storage_config();
    protected:
        //! Open up a Storage engine.
        /*!
         \par
         Hidden. Use the Storage_factory instead.
         \sa lj::Storage_factory
         \param name The document repository name.
         \param server_config The configuration for the storage server.
         */
        Storage(const std::string &name,
                const lj::Bson* const server_config);
        
    private:
        lj::Vault* vault_;
        std::map<std::string, lj::Index*> indices_;
        lj::Bson* storage_config_;
        const lj::Bson* const server_config_;
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
