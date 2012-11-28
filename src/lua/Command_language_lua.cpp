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
        lua::Bson* response = lua::Lunar<lua::Bson>::check(L,
                lua_upvalueindex(1));

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
        response->node().push_child("output",
                lj::bson::new_string(buffer.str()));
        lua_pop(L, 1); // remove tostring function.
        return 0;
    }

    int change_adapt_language(lua_State* L)
    {
        lua::Bson* response = lua::Lunar<lua::Bson>::check(L,
                lua_upvalueindex(1));

        std::string language(lua::as_string(L, -1));
        response->node().set_child("next_language",
                lj::bson::new_string(language));
        lua_pop(L, 1); // remove the language.
        return 0;
    }

    int disconnect(lua_State* L)
    {
        lua::Bson* response = lua::Lunar<lua::Bson>::check(L,
                lua_upvalueindex(1));

        response->node().set_child("disconnect", lj::bson::new_boolean(true));

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

    int simple_assert(lua_State* L)
    {
        int top = lua_gettop(L);

        // Ignore empty asserts.
        if (top == 0)
        {
            return 0;
        }
        else if (top > 2)
        {
            luaL_error(L, "Assert called with too many args.");
        }

        // First argument must be a boolean.
        if (!lua_isboolean(L, 1))
        {
            luaL_error(L, "Assert requires a boolean type.");
        }

        if (lua_toboolean(L, 1) == true)
        {
            lua_pop(L, top);
        }
        else
        {
            if (top == 1)
            {
                luaL_error(L, "Assert failed.");
            }
            else
            {
                // top of the stack is already the error message.
                lua_error(L);
            }
        }

        return 0;
    }
};

namespace lua
{
    Command_language_lua::Command_language_lua(logjamd::Connection* conn,
            lj::bson::Node* req) :
            connection_(conn),
            request_(req),
            L(luaL_newstate()),
            state_(new Bson(connection_->state()))
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

        lua_pushcfunction(L, &simple_assert);
        lua_setglobal(L, "ASSERT");

        // Put the connection state in the scope.
        Lunar<Bson>::push(L, state_, false);
        lua_setglobal(L, "SESSION");

        // Put the request into the scope.
        Lunar<Bson>::push(L, new Bson(*request_), true);
        lua_setglobal(L, "REQUEST");
    }

    Command_language_lua::~Command_language_lua()
    {
        connection_->state().copy_from(state_->node());
        delete state_;
        lua_close(L);
    }

    bool Command_language_lua::perform(lj::bson::Node& response)
    {
        // Setup replaced methods.
        std::unique_ptr<Bson> response_wrapper(new Bson(response));
        Lunar<Bson>::push(L, response_wrapper.get(), false);
        lua_pushvalue(L, -1);
        lua_pushvalue(L, -1);
        lua_pushcclosure(L, &print_to_response, 1);
        lua_setglobal(L, "print");
        lua_pushcclosure(L, &change_adapt_language, 1);
        lua_setglobal(L, "change_language");
        lua_pushcclosure(L, &disconnect, 1);
        lua_setglobal(L, "exit");

        // Put the response into the scope.
        Lunar<Bson>::push(L, response_wrapper.get(), false);
        lua_setglobal(L, "RESPONSE");

        std::string cmd(lj::bson::as_string(request_->nav("command")));
        luaL_loadbuffer(L,
                cmd.c_str(),
                cmd.size(),
                "command");
        int err = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (0 != err)
        {
            std::string error_msg(as_string(L, -1));
            response_wrapper->node().set_child("message",
                    lj::bson::new_string(error_msg));
            response_wrapper->node().set_child("success",
                    lj::bson::new_boolean(false));
        }

        response.copy_from(response_wrapper->node());

        bool keep_alive = true;
        if (response.exists("disconnect"))
        {
            response.set_child("disconnect", NULL);
            keep_alive = false;
        }
        return keep_alive;
    }

    std::string Command_language_lua::name()
    {
        return "Lua";
    }
}; // namespace lua
