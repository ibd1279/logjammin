/*!
 \file logjamd/Server_secure.cpp
 \brief Logjam server networking implementation.
 \author Jason Watson

 Copyright (c) 2011, Jason Watson
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

#include "logjamd/Connection_secure.h"
#include "logjamd/Server_secure.h"
#include "lj/Streambuf_bio.h"
#include "lj/Exception.h"
#include <algorithm>
#include <mutex>
#include <thread>

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
}

namespace logjamd
{
    Server_secure::Server_secure(lj::bson::Node* config) :
            logjamd::Server(config),
            io_(NULL),
            running_(false),
            connections_()
    {
    }

    Server_secure::~Server_secure()
    {
        if (io_)
        {
            BIO_free(io_);
        }

        lj::log::format<lj::Debug>("Deleting all connections for server %p")
                << (const void*)this
                << lj::log::end;
        for (auto iter = connections_.begin();
                connections_.end() != iter;
                ++iter)
        {
            delete (*iter);
        }
    }

    void Server_secure::startup()
    {
        std::string listen(lj::bson::as_string(cfg()["server/listen"]));

        // Start port listening.
        char* host_port = new char[listen.size() + 1];
        memcpy(host_port, listen.c_str(), listen.size() + 1);

        // allocate the listener.
        io_ = BIO_new_accept(host_port);
        if (!io_)
        {
            std::string msg("Unable to allocate IO object. ");
            throw LJ__Exception(msg + lj::openssl_get_error_string());
        }

        // First accept sets things up, doesn't actually accept a connection.
        if (BIO_do_accept(io_) <= 0)
        {
            std::string msg("Unable to setup the listener object. ");
            throw LJ__Exception(msg + lj::openssl_get_error_string());
        }
    }

    void Server_secure::listen()
    {
        running_ = true;
        while(running_)
        {
            // Accept a connection.
            if (BIO_do_accept(io_) <= 0)
            {
                std::string msg("Unable to accept a connection. ");
                throw LJ__Exception(msg + lj::openssl_get_error_string());
            }

            // Collect all the things we need for this connection.
            lj::bson::Node* connection_state = new lj::bson::Node();
            lj::Streambuf_bio<char>* connection_buffer = 
                    new lj::Streambuf_bio<char>(BIO_pop(io_), 4096, 4096);

            // Create the new connection
            Connection_secure* connection = new Connection_secure(
                    this,
                    connection_state,
                    connection_buffer);

            // Kick off the thread for that connection.
            connection->start();

            // store a copy locally for management.
            // TODO this needs to be flushed out 
            connections_.push_back(connection);
        }
    }

    void Server_secure::shutdown()
    {
        running_ = false;
    }

    void Server_secure::detach(Connection* conn)
    {
        // remove this pointer from the collection of managed connections.
        Connection_secure* ptr = dynamic_cast<logjamd::Connection_secure*>(
                conn);
        connections_.remove(ptr);
    }
};
