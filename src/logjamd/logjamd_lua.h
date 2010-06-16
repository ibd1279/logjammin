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

#include <string>

namespace logjamd {
    //! Initialize the lua state for the server process.
    /*!
     \param L The lua state.
     */
    void logjam_lua_init(lua_State *L);
    
    //! Initialize the lua state for the server process.
    /*!
     \param L The lua state.
     \param name The connection identifier.
     */
    void logjam_lua_init_connection(lua_State *L, const std::string& name);
        
    //! Load the connection configuration.
    /*!
     \par
     Returns the Lua_bson connection configuration object.
     \param L The lua state.
     \return 1
     */
    int connection_config_load(lua_State* L);
    
    //! Save the connection configuration.
    /*!
     \par
     Pops the connection configuration (Lua_bson) off the stack.
     \param L The lua state.
     \return 0
     */
    int connection_config_save(lua_State* L);
    
    //! Add a default storage engine.
    /*!
     \par
     Pops the storage name (Lua string) off the stack.
     \par
     Pops the connection configuration (Lua_bson) off the stack.
     \param L The lua state.
     \return 0
     */
    int connection_config_add_default_storage(lua_State* L);
    
    //! Remove a default storage engine.
    /*!
     \par
     Pops the storage name (Lua string) off the stack.
     \par
     Pops the connection configuration (Lua_bson) off the stack.
     \param L The lua state.
     \return 0
     */
    int connection_config_remove_default_storage(lua_State* L);
    
    //! Put a result set on the response.
    /*!
     \par
     Populates the item field on the response.
     \par
     Expects a Lua_record_set object on top of the stack.
     \param L The lua state.
     \return 0
     */
    int send_set(lua_State* L);
    
    //! Put one item on the response.
    /*!
     \par
     populates the item field on the response with a single item.
     \par
     Expects a Lua_bson object on top of the stack.
     \param L The lua state.
     \return 0
     */
    int send_item(lua_State* L);
};