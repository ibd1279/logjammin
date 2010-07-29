#pragma once
/*!
 \file Storage_factory.h
 \brief LJ Storage_factor header.
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

#include "lj/bson.h"
#include <string>
#include <map>

namespace lj
{
    class Storage;
    
    //! Factory to produce storage objects.
    /*!
     Produces storage objects. Treats the objects as singletons,
     \author Jason Watson
     \version 1.0
     \date May 21, 2010
     \sa lj::Storage
     */
    class Storage_factory {
    public:
        //! Produce a Storage object.
        /*!
         \param name The name of the Storage object.
         \param server_config The server configuration.
         \return Reference to the storage object.
         */
        static Storage* produce(const std::string& name, const lj::Bson& server_config);
        
        //! Force a specific Storage object to be re-produced.
        /*!
         \param name The name of the Storage object.
         \param server_config The server configuration.
         */
        static void recall(const std::string& name, const lj::Bson& server_config);
        
        //! Recall and produce a storage object in a single call.
        /*!
         \param name The name of the Storage object.
         \param server_config The server configuration.
         \return Pointer to the storage object.
         */
        static Storage* reproduce(const std::string& name, const lj::Bson& server_config);
        
        //! Checkpoint all databases.
        static void checkpoint_all();
    private:
        typedef std::map<std::string, Storage*> Cache_map;
        static Cache_map cache_;
    };
};