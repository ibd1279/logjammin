#pragma once
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

#include "lj/Bson.h"
#include "lj/Logger.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include <cstring>
#include <string>

namespace logjamd
{
    void set_logging_levels(const lj::Bson& config);

    //! Initialize the lua state for the server process.
    /*!
     \par
     This is primarily responsible for getting the cached environment for
     a connection, and leaving it ontop of the stack.
     \param L The lua state.
     \param config The data directory.
     */
    void logjam_lua_init(lua_State* L, lj::Bson* config);
    
    //! Execute an event.
    void get_event(lua_State* L, const std::string& db_name, const std::string& event);
    
    //! Push a bson object onto the replication queue and return the name.
    const std::string push_replication_record(lua_State* L, const lj::Bson& b);
    
    void push_replication_command(lua_State* L,
                                  const std::string& action,
                                  const std::string& dbname,
                                  const std::string& obj);    
    
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
};
