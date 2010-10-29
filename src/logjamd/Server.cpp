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
        config_ = lj::bson_load(data_dir_ + "/config");
    }
    
    Server::~Server()
    {
        lua_close(lua_);
        delete config_;
    }
    
    lj::Socket_dispatch* Server::accept(int socket, const std::string& remote_ip)
    {
        Connection* client = new Connection(remote_ip, lua_, config_, data_dir_);
        client->set_socket(socket);
        client->set_mode(Socket_dispatch::k_communicate);
        return client;
    }
    
    void Server::read(const char* buffer, int sz)
    {
    }
    
}; // namespace logjamd