/*
 \file logjam/Network_socket.cpp
 \brief Logjam Network socket implementation.
 \author Jason Watson

 Copyright (c) 2012, Jason Watson
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

#include "logjam/Network_socket.h"
#include "logjam/Network_address_info.h"
#include "lj/Exception.h"
#include <sstream>
#include <utility>
extern "C" {
#include <stdio.h>
#include <unistd.h>
}

namespace logjam
{
    Network_socket::Network_socket() :
            lj::medium::Socket(-1),
            is_open_(false)
    {
    }
    
    Network_socket::Network_socket(int socket) :
            lj::medium::Socket(socket),
            is_open_(true)
    {
    }

    Network_socket::Network_socket(Network_socket&& o) :
            lj::medium::Socket(o.fd_),
            is_open_(o.is_open_)
    {
        o.is_open_ = false;
        o.fd_ = -1;
    }

    Network_socket& Network_socket::operator=(Network_socket&& rhs)
    {
        std::swap(rhs.is_open_, is_open_);
        std::swap(rhs.fd_, fd_);
        return *this;
    }

    Network_socket::~Network_socket()
    {
        close();
    }
    
    void Network_socket::close()
    {
        if (is_open())
        {
            lj::log::format<lj::Info>("Closing fh %d").end(socket());
            ::close(socket());
            is_open_ = false;
            fd_ = -1;
        }
    }
    
    int Network_socket::socket() const
    {
        return fd_;
    }

    Network_socket socket_for_target(const struct addrinfo& target)
    {
        // Get the socket object and deal with the C error states.
        int sockfd = ::socket(target.ai_family,
                target.ai_socktype,
                target.ai_protocol);
        if (0 > sockfd)
        {
            std::ostringstream oss;
            oss << "Unable to create the socket to ["
                    << logjam::Network_address_info::as_string(target.ai_addr)
                    << "]. ["
                    << ::strerror(errno)
                    << "]";
            throw LJ__Exception(oss.str());
        }

        int result = ::connect(sockfd,
                target.ai_addr,
                target.ai_addrlen);
        if (0 > result)
        {
            std::ostringstream oss;
            oss << "Unable to connect to ["
                    << logjam::Network_address_info::as_string(target.ai_addr)
                    << "]. ["
                    << ::strerror(errno)
                    << "]";
            ::close(sockfd);
            throw LJ__Exception(oss.str());
        }

        return Network_socket(sockfd);
    }
}; // namespace logjam
