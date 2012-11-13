/*
 \file logjam/Network_address_info.cpp
 \brief Logjam Network Address Info implementation.
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

#include "Network_address_info.h"

namespace logjam
{
    Network_address_info::Network_address_info(const std::string& host,
            const std::string& port,
            int flags,
            int family,
            int type,
            int protocol) :
            info_(nullptr),
            current_(nullptr),
            status_(0)
    {
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_flags = flags;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        
        // If the host is "*", then we aren't going to be picky.
        if (host.compare("*") == 0)
        {
            status_ = getaddrinfo(nullptr, port.c_str(), &hints, &info_);
        }
        else
        {
            status_ = getaddrinfo(host.c_str(), port.c_str(), &hints, &info_);
        }
    }
    Network_address_info::Network_address_info(const std::string& port,
            int flags,
            int family,
            int type,
            int protocol) :
            info_(nullptr),
            current_(nullptr),
            status_(0)
    {
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_flags = flags;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        
        // If the port contains a colon, assume it is a host + port value.
        int col_pos = port.find_first_of(':');
        if (col_pos == std::string::npos)
        {
            status_ = getaddrinfo(nullptr, port.c_str(), &hints, &info_);
        }
        else
        {
            status_ = getaddrinfo(port.substr(0, col_pos).c_str(),
                    port.substr(col_pos + 1).c_str(),
                    &hints,
                    &info_);
        }
    }


    Network_address_info::Network_address_info(Network_address_info&& o) :
            info_(o.info_),
            current_(o.current_),
            status_(o.status_)
    {
        o.info_ = nullptr;
        o.current_ = nullptr;
    }

    Network_address_info::~Network_address_info()
    {
        if (info_)
        {
            freeaddrinfo(info_);
        }
    }

    Network_address_info& Network_address_info::operator=(Network_address_info&& o)
    {
        std::swap(info_, o.info_);
        std::swap(current_, o.current_);
        std::swap(status_, o.status_);
        return *this;
    }

    bool Network_address_info::next()
    {
        if (info_ == nullptr || status_ != 0)
        {
            return false;
        }
        else if (current_ == nullptr)
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

    struct addrinfo& Network_address_info::current()
    {
        return *current_;
    }

    std::string Network_address_info::error()
    {
        return std::string(gai_strerror(status_));
    }

    std::string Network_address_info::as_string(struct sockaddr* sa)
    {
        // Beej gets credit for this function:
        // http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleserver
        char ip[INET6_ADDRSTRLEN];

        if (sa->sa_family == AF_INET)
        {
            inet_ntop(sa->sa_family,
                    &(((struct sockaddr_in*)sa)->sin_addr),
                    ip,
                    INET6_ADDRSTRLEN);
        }
        else
        {
            inet_ntop(sa->sa_family,
                    &(((struct sockaddr_in6*)sa)->sin6_addr),
                    ip,
                    INET6_ADDRSTRLEN);
        }

        return std::string(ip);
    }
}; // namespace logjam