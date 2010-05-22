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
         sc_add_nested(role_cfg, "allowed")
         sc_save("role", role_cfg)         
         \endcode
         \param name The name of the Storage object.
         \return Pointer to the storage object.
         */
        static Storage* produce(const std::string& name);
        
        //! Force a specific Storage object to be re-produced.
        /*!
         \par
         All existing Storage object pointers for the \c name Storage object
         will become invalid.
         \todo
         Some type of abstraction layer to allow "reconnecting" after a recall
         would make this much more useful.
         \param name The name of the Storage object.
         */
        static void recall(const std::string& name);
    private:
        typedef std::map<std::string, Storage*> Cache_map;
        static Cache_map cache_;
    };
};