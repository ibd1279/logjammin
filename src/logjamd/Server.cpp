/*!
 \file Server.cpp
 \brief Logjam server networking implementation.
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

#include "logjamd/Server.h"

#include "logjamd/Connection.h"
#include "logjamd/logjamd_lua.h"
#include "logjamd/Lua_bson.h"
#include "lj/Bson.h"
#include "lj/Logger.h"
#include "lj/Time_tracker.h"

extern "C" {
#include "lualib.h"
}
#include <sstream>
#include <list>
#include <sys/time.h>

namespace
{
    //! Create a new lua thread and sandbox the global environment.
    lua_State* prepare_lua_environment(lua_State *server_lua,
                                       const std::string& identifier)
    {
        lua_State* L = lua_newthread(server_lua); // {L}
        lua_getglobal(L, "environment_cache"); // {L, ec}
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1); // {L}
            lua_newtable(L); // {L, ec}
            lua_pushvalue(L, -1); // {L, ec, ec}
            lua_setglobal(L, "environment_cache"); // {L, ec}
        }
        lua_pushstring(L, identifier.c_str()); // {L, ec, name}
        lua_gettable(L, -2); // {L, ec, t}
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1); // {L, ec}
            lua_newtable(L); // {L, ec, t}
            lua_pushstring(L, identifier.c_str()); // {L, ec, t, name}
            lua_pushvalue(L, -2); // {L, ec, t, name, t}
            lua_settable(L, -4); // {L, ec, t}
            lua_pushvalue(L, -1); // {L, ec, t, t}
            lua_pushstring(L, "__index"); // {L, ec, t, t, __index}
            lua_pushvalue(L, LUA_GLOBALSINDEX); // {L, ec, t, t, __index, _G}
            lua_settable(L, -3); // {L, ec, t, t}
            lua_setmetatable(L, -2); // {L, ec, t}
        }
        lua_replace(L, -2); // {L, t}
        lua_setfenv(L, -2); // {L}
        lua_pop(L, 1); // {}

        return L;
    }
} // namespace

namespace logjamd
{
    Server::Server(const std::string& data_directory) : lua_(0), config_(0), data_dir_(data_directory)
    {
        set_mode(Socket_dispatch::k_listen);

        // prepare lua.
        lua_ = luaL_newstate();
        luaL_openlibs(lua_);
        logjam_lua_init(lua_, data_dir_);

        // Load configuration.
    }
    
    Server::~Server()
    {
        lua_close(lua_);
        delete config_;
    }
    
    lj::Socket_dispatch* Server::accept(int socket, const std::string& remote_ip)
    {
        lua_State* L = prepare_lua_environment(lua_, remote_ip);
        Connection* client = new Connection(remote_ip, L, config_, data_dir_);
        client->set_socket(socket);
        client->set_mode(Socket_dispatch::k_communicate);
        return client;
    }
    
    void Server::read(const char* buffer, int sz)
    {
    }
    
}; // namespace logjamd
