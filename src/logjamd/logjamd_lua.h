/*!
 \file logjamd_lua.h
 \brief Logjam server lua functions header.
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
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

namespace logjamd {
    void register_logjam_functions(lua_State *L);
    
    int connection_config_load(lua_State* L);
    int connection_config_save(lua_State* L);
    int connection_config_add_default_storage(lua_State* L);
    int connection_config_remove_default_storage(lua_State* L);
    
    //! Create a new lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     New configuration is populated with some default fields.
     \par
     Pops the storage name (lua string) off the stack.
     \par
     Pushes the new Lua_bson object onto the stack.
     \param L The lua state.
     \returns 1.
     */
    int storage_config_new(lua_State* L);
    
    //! Save a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Saves the configuration for the provided storage name.
     \par
     Pops the storage configuration document (Lua_bson) off the stack.
     \par
     Pops the storage name (lua string) off the stack.
     \param L The lua state.
     \returns 0
     */
    int storage_config_save(lua_State* L);
    
    //! Load a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Loads the configuration for the provided storage name.
     \par
     Pops the storage name (lua string) off the stack.
     \par
     Pushes the new Lua_bson object onto the stack.
     \param L The lua state.
     \returns 1.
     */
    int storage_config_load(lua_State* L);
    
    //! Add an index to a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Adds an index to the Configuration document.
     \par
     Pops the index comparison type (lua string) off the stack.
     \par
     Pops the field name to index (lua string) off the stack.
     \par
     Pops the name of the index (lua string) off the stack.
     \par
     Pops the index type off (lua string) off the stack.
     \par
     Pops the Storage configuration document (Lua_bson) off the stack.
     \param L The lua state.
     \returns 0.
     */
    int storage_config_add_index(lua_State* L);
    
    //! Add a nested field marker to a lj::Storage configuration document.
    /*!
     \par
     Storage configuration is a lj::Bson object.
     \par
     Notifies the server that the provided field should index its children.
     \par
     Pops the field name to index (lua string) off the stack.
     \par
     Pops the Storage configuration document (Lua_bson) off the stack.
     \param L The lua state.
     \returns 0.
     */
    int storage_config_add_nested_field(lua_State* L);
    
    //! Send a response.
    /*!
     \par
     Populates the item field on the response.
     \par
     Expects a Lua_record_set object on top of the stack.
     \param L The lua state.
     \return 0
     */
    int send_response(lua_State* L);
};