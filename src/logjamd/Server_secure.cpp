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
#include "logjam/Network_address_info.h"
#include "lj/Exception.h"
#include "lj/Streambuf_bsd.h"
#include <algorithm>
#include <mutex>
#include <thread>

extern "C"
{
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
}

namespace
{
    unsigned int k_dh_bits = 2048;
}

namespace logjamd
{
    Server_secure::Server_secure(lj::bson::Node* config) :
            logjamd::Server(config),
            io_(-1),
            running_(false),
            connections_(),
            credentials_(),
            key_exchange_(k_dh_bits)
    {
    }

    Server_secure::~Server_secure()
    {
        if (-1 < io_)
        {
            ::close(io_);
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
        // Link the key exchange and the credentials.
        credentials_.configure_key_exchange(key_exchange_);

        //std::string listen(lj::bson::as_string(cfg()["server/listen"]));

        // TODO this needs to translate from the config string above to the
        // parameters needed below.
        logjam::Network_address_info info("12345",
                AI_PASSIVE,
                AF_UNSPEC,
                SOCK_STREAM,
                0);
        if (!info.next())
        {
            // we didn't get any address information back, so abort!
            throw LJ__Exception(info.error());
        }

        // Now create my socket descriptor for listening.
        io_ = ::socket(info.current().ai_family,
                info.current().ai_socktype,
                info.current().ai_protocol);
        if (0 > io_)
        {
            // Did not get a socket descriptor.
            throw LJ__Exception(strerror(errno));
        }

        int rc = ::bind(io_,
                info.current().ai_addr,
                info.current().ai_addrlen);
        if (0 > rc)
        {
            // Did not get a socket descriptor.
            throw LJ__Exception(strerror(errno));
        }

        rc = ::listen(io_, 5);
        if (0 > rc)
        {
            // did not bind the listener to a port.
            throw LJ__Exception(strerror(errno));
        }
    }

    void Server_secure::listen()
    {
        running_ = true;
        while(running_)
        {
            // Accept a connection.
            struct sockaddr_storage remote_addr;
            socklen_t remote_addr_size = sizeof(struct sockaddr_storage);
            int client_socket = accept(io_,
                    (struct sockaddr *)&remote_addr,
                    &remote_addr_size);
            if (0 > client_socket)
            {
                // I had problems accepting that client.
                throw LJ__Exception(strerror(errno));
            }

            // Collect all the things we need for this connection.
            lj::bson::Node* connection_state = new lj::bson::Node();
            std::string remote_ip = logjam::Network_address_info::as_string(
                    (struct sockaddr*)&remote_addr);
            connection_state->set_child("client/address",
                    lj::bson::new_string(remote_ip));

            lj::log::format<lj::Info>("Accepted a connection form %s.")
                    << remote_ip
                    << lj::log::end;

            // Create the new connection
            Connection_secure* connection = new Connection_secure(
                    this,
                    connection_state,
                    client_socket);

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

    std::unique_ptr<Server_secure::Session> Server_secure::new_session(int socket_descriptor)
    {
        // see http://www.gnu.org/software/gnutls/manual/gnutls.html#Echo-server-with-anonymous-authentication
        std::unique_ptr<Server_secure::Session> session(
                new Server_secure::Session(Server_secure::Session::k_server));
        session->credentials().set(&credentials_);
        session->set_cipher_priority("NORMAL:+ANON-ECDH:+ANON-DH");
        session->set_dh_prime_bits(key_exchange_.bits());
        session->set_socket(socket_descriptor);
        return session;
    }
};
