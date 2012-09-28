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
    class Address_info
    {
    public:
        Address_info(const char* host,
                const char* port,
                int flags,
                int family,
                int type,
                int protocol) :
                info_(nullptr),
                current_(nullptr)
        {
            struct addrinfo hints;
            std::memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_flags = flags;
            hints.ai_family = family;
            hints.ai_socktype = type;
            hints.ai_protocol = protocol;
            status_ = getaddrinfo(host, port, &hints, &info_);
        }
        Address_info(const Address_info& o) = delete;
        Address_info(Address_info&& o) :
                status_(o.status_),
                info_(o.info_),
                current_(o.current_)
        {
            o.info_ = nullptr;
            o.current_ = nullptr;
        }
        ~Address_info()
        {
            if (info_)
            {
                freeaddrinfo(info_);
            }
        }
        Address_info& operator=(const Address_info& o) = delete;
        Address_info& operator=(Address_info&& o)
        {
            std::swap(info_, o.info_);
            std::swap(current_, o.current_);
            std::swap(status_, o.status_);
            return *this;
        }
        bool next()
        {
            if (current_ == nullptr)
            {
                current_ = info_;
                return true;
            }
            else if (current_->ai_next)
            {
                current_ = current_->ai_next;
                return true;
            }
            return false;
        }
        struct addrinfo& current()
        {
            return *current_;
        }
        std::string error()
        {
            return std::string(gai_strerror(status_));
        }
    private:
        int status_;
        struct addrinfo* info_;
        struct addrinfo* current_;
    };

    // get sockaddr, IPv4 or IPv6:
    void* get_in_addr(struct sockaddr* sa)
    {
        if (sa->sa_family == AF_INET) {
            return &(((struct sockaddr_in*)sa)->sin_addr);
        }

        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
}

namespace logjamd
{
    Server_secure::Server_secure(lj::bson::Node* config) :
            logjamd::Server(config),
            io_(-1),
            running_(false),
            connections_()
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
        //std::string listen(lj::bson::as_string(cfg()["server/listen"]));

        // TODO this needs to translate from the config string above to the
        // parameters needed below.
        Address_info info(nullptr,
                "12345",
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
            char remote_ip[INET6_ADDRSTRLEN];
            inet_ntop(remote_addr.ss_family,
                    get_in_addr((struct sockaddr*)&remote_addr),
                    remote_ip,
                    INET6_ADDRSTRLEN);
            connection_state->set_child("client/address",
                    lj::bson::new_string(remote_ip));

            lj::log::format<lj::Info>("Accepted a connection form %s.")
                    << remote_ip
                    << lj::log::end;

            // Build the objects used for communication abstraction.
            lj::medium::Socket* medium = new lj::medium::Socket(client_socket);
            lj::Streambuf_bsd<lj::medium::Socket>* connection_buffer =
                    new lj::Streambuf_bsd<lj::medium::Socket>(medium, 8192, 8192);

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
