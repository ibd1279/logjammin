/*!
 \file lua/Command_language_lua.cpp
 \brief Logjam server networking header.
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

#include "lua/Command_language_lua.h"
#include "logjamd/Connection.h"

namespace
{
    int print_to_response(lua_State* L)
    {
        // TODO Need to add some type checking in general.
        lj::bson::Node* response =
                static_cast<lj::bson::Node*>(lua_touserdata(L,
                        lua_upvalueindex(1)));

        // TODO This needs to be updated to do more than just the first arg.
        // TODO getting a string is pretty common. Should be moved to a util.
        const char* ptr = luaL_checkstring(L, -1);
        if (ptr != NULL)
        {
            size_t l = lua_strlen(L, -1);
            std::string tmp(ptr, l);
            response->push_child("output", lj::bson::new_string(tmp));
        }
        else
        {
            response->push_child("output", lj::bson::new_string(""));
        }
        lua_pop(L, 1);
        return 0;
    };
};

namespace lua
{
    Command_language_lua::Command_language_lua(logjamd::Connection* conn,
            lj::bson::Node* req) :
            connection_(conn),
            request_(req),
            L(lua_open())
    {
        luaL_openlibs(L);
        // Register my extensions.
        // Put the connection state in the scope.
        // Put the request into the scope.
    }

    Command_language_lua::~Command_language_lua()
    {
        lua_close(L);
    }

    void Command_language_lua::perform(lj::bson::Node& response)
    {
        // Setup replaced methods.
        lua_pushlightuserdata(L, &response);
        lua_pushcclosure(L, &print_to_response, 1);
        lua_setglobal(L, "print");

        // Put the response into the scope.
        // TODO make a lua bson wrapper.

        std::string cmd(lj::bson::as_string(request_->nav("command")));
        luaL_loadbuffer(L,
                cmd.c_str(),
                cmd.size(),
                "command");
        lua_pcall(L, 0, LUA_MULTRET, 0);
    }

    std::string Command_language_lua::name()
    {
        return "Lua";
    }
}; // namespace lua
