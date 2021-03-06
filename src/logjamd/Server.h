#pragma once
/*!
 \file logjamd/Server.h
 \brief Logjam server networking header.
 \author Jason Watson

 Copyright (c) 2014, Jason Watson
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
#include <string>

namespace logjamd
{
    class Connection;

    //! Abstract base class for accepting connections.
    class Server {
    public:
        //! Construct a new Server.
        /*!
         \par
         All servers are required to have some form of configuration
         \param config The configuration for the server.
         */
        Server(lj::bson::Node* config) : config_(config)
        {
        }

        //! Destructor
        virtual ~Server()
        {
            if (config_)
            {
                delete config_;
            }
        }

        //! Perform any initialization necessary for the server.
        virtual void startup() = 0;
        
        //! Start listening for connections.
        virtual void listen() = 0;

        //! Attempt a graceful shutdown.
        virtual void shutdown() = 0;

        //! Detach a connection from the server.
        /*!
         \par
         This allows the server to stop managing the connection.
         including not shutting down the connection when the server is
         shutdown.
         \param conn The connection to detach.
         */
        virtual void detach(Connection* conn) = 0;

        //! Read only configuration.
        virtual const lj::bson::Node& cfg() const
        {
            return *config_;
        }

        //! Read/write configuration.
        virtual lj::bson::Node& config()
        {
            return *config_;
        }
    private:
        lj::bson::Node* config_;
    };
};
