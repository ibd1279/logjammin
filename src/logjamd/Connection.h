#pragma once
/*!
 \file Connection.h
 \brief Logjam server connection to a client definition.
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
#include "lj/Sockets.h"

extern "C" {
#include "lualib.h"
}

namespace logjamd
{
    // forward declare Stage
    class Stage;

    //! Server to client connection.
    /*!
     \par
     This class represents how the server keeps track of the command
     processor and its changing state.
     \author Jason Watson
     \version 1.0
     \date October 26, 2010
     */
    class Connection : public lj::Socket_dispatch {
    public:

        //! Create a new connection object.
        /*!
         \param client_ip The ip address of the client.
         \param client_lua The lua state for the client.
         \param server_config The server configuration.
         \param data_directory The data directory.
         */
        Connection(const std::string& client_ip,
                   lua_State* client_lua,
                   const lj::Bson* server_config,
                   const std::string& data_directory);

        //! Destructor.
        virtual ~Connection();
        virtual lj::Socket_dispatch* accept(int socket,
                                            const std::string& buffer);
        virtual void read(const char* buffer, int sz);

        //! Get the ip address of the client.
        inline const std::string& ip() const { return ip_; };

        //! Get the server configuration.
        inline const lj::Bson& server_config() const { return *server_config_; };

        //! Get the server lua object.
        inline lua_State* lua() { return lua_; };
        
        //! Get the data directory path.
        inline const std::string& data_directory() const { return data_dir_; };
    private:
        char * in_;
        int in_offset_;
        int in_sz_;
        bool in_post_length_;

        const std::string ip_;
        lua_State* lua_;
        const lj::Bson* server_config_;
        const std::string& data_dir_;

        Stage* stage_;
    };
};


