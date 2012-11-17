/*
 \file logjam/Network_connection.cpp
 \brief Logjam Network connection implementation.
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

#include "logjam/Network_connection.h"
#include "lj/Exception.h"
#include <sstream>
extern "C" {
#include <stdio.h>
#include <unistd.h>
}

namespace logjam
{

    Network_connection::Network_connection() :
            is_open_(false),
            socket_(-1)
    {
    }
    
    Network_connection::Network_connection(int socket) :
            is_open_(true),
            socket_(socket)
    {
    }

    Network_connection::Network_connection(Network_connection&& orig) :
            is_open_(orig.is_open_),
            socket_(orig.socket_)
    {
        orig.is_open_ = false;
        orig.socket_ = -1;
    }

    Network_connection::~Network_connection()
    {
        close();
    }
    
    Network_connection& Network_connection::operator=(Network_connection&& orig)
    {
        // backup current values.
        bool tmp_is_open = is_open_;
        int tmp_socket = socket_;

        // Copy over the new values
        is_open_ = orig.is_open_;
        socket_ = orig.socket_;

        // restore old values into orig object.
        orig.is_open_ = tmp_is_open;
        orig.socket_ = tmp_socket;
        
        return *this;
    }
    
    void Network_connection::connect(const struct addrinfo& target)
    {
        // If the connection is already open, than someone made a mistake somewhere.
        if (is_open())
        {
            throw LJ__Exception("Connection already open. Cannot reconnect.");
        }
        
        // Get the socket object and deal with the C error states.
        int sockfd = ::socket(target.ai_family,
                target.ai_socktype,
                target.ai_protocol);
        if (0 > sockfd)
        {
            std::ostringstream oss;
            oss << "Unable to create the socket. ["
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
            oss << "Unable to connect. ["
                    << ::strerror(errno)
                    << "]";
            ::close(sockfd);
            throw LJ__Exception(oss.str());
        }
        
        is_open_ = true;
        socket_ = sockfd;
    }
    
    void Network_connection::close()
    {
        if (is_open())
        {
            ::close(socket_);
            is_open_ = false;
            socket_ = -1;
        }
    }
    
    int Network_connection::socket() const
    {
        if (!is_open())
        {
            throw LJ__Exception("Socket is not open.");
        }
        
        return socket_;
    }
}; // namespace logjam