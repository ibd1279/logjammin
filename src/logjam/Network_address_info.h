#pragma once
/*
 \file logjam/Network_address_info.h
 \brief Logjam Network Address Info header.
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

extern "C"
{
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
}

#include <string>

namespace logjam
{
    //! Network address information
    /*!
     Provides a simple wrapper around the addrinfo stuff. Implements an
     enumerable style interface to allow iterating over multiple results.
     \since 0.2
     */
    class Network_address_info
    {
    public:
        Network_address_info(const std::string& host,
                const std::string& port,
                int flags,
                int family,
                int type,
                int protocol);
        Network_address_info(const std::string& port,
                int flags,
                int family,
                int type,
                int protocol);
        Network_address_info(const Network_address_info& o) = delete;
        Network_address_info(Network_address_info&& o);
        ~Network_address_info();
        Network_address_info& operator=(const Network_address_info& o) = delete;
        Network_address_info& operator=(Network_address_info&& o);
        bool next();
        struct addrinfo& current();
        std::string error();
        
        //! Helper method for converting a sockaddr into a string.
        static std::string as_string(struct sockaddr* sa);
    private:
        struct addrinfo* info_;
        struct addrinfo* current_;
        int status_;
    }; // class logjam::Network_address_info
}; // namespace logjam