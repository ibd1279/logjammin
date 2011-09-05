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

#include "logjamd/Connection.h"
#include "lua/Bson.h"
#include "lua/Command_language_lua.h"
#include "lua/Document.h"
#include "lua/Uuid.h"
#include <sstream>

namespace
{
    int print_to_response(lua_State* L)
    {
        lj::bson::Node* response = static_cast<lj::bson::Node*>(
                lua_touserdata(L, lua_upvalueindex(1)));

        int top = lua_gettop(L);
        std::ostringstream buffer;
        lua_getglobal(L, "tostring");
        for (int i = 1; i <= top; ++i)
        {
            lua_pushvalue(L, -1);
            lua_pushvalue(L, i);
            lua_call(L, 1, 1);

            if (i > 1)
            {
                buffer << "\t";
            }
            buffer << lua::as_string(L, -1);
            lua_pop(L, 1);
        }
        response->push_child("output", lj::bson::new_string(buffer.str()));
        lua_pop(L, 1); // remove tostring function.
        return 0;
    }
    int get_crypto_key(lua_State* L)
    {
        logjamd::Connection* connection = static_cast<logjamd::Connection*>(
                lua_touserdata(L, lua_upvalueindex(1)));
        std::string identifier(lua::as_string(L, -1));
        int sz;
        const void* data = connection->get_crypto_key(identifier, &sz);

        if (data)
        {
            std::unique_ptr<lj::bson::Node> ptr(lj::bson::new_binary(
                    static_cast<const uint8_t*>(data),
                    sz,
                    lj::bson::Binary_type::k_bin_user_defined));
            lua::Bson_ro* key_data = new lua::Bson_ro(*ptr);
            lua::Lunar<lua::Bson_ro>::push(L, key_data, true);
        }
        else
        {
            // if the key was unknown, push nil.
            lua_pushnil(L);
        }

        return 1;
    }
};

namespace lua
{
    Command_language_lua::Command_language_lua(logjamd::Connection* conn,
            lj::bson::Node* req) :
            connection_(conn),
            request_(req),
            L(lua_open())
    {
        // Standard libraries.
        luaL_openlibs(L);

        // Register my extensions.
        Lunar<Bson>::Register(L);
        Lunar<Bson_ro>::Register(L);
        Lunar<Document>::Register(L);
        Lunar<Uuid>::Register(L);

        // One-off functions.
        lua_pushlightuserdata(L, conn);
        lua_pushcclosure(L, &get_crypto_key, 1);
        lua_setglobal(L, "get_crypto_key");

        // XXX Put the connection state in the scope.
        // XXX Put the request into the scope.
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
