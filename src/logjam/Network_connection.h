#pragma once
/*
 \file logjam/Network_connection.h
 \brief Logjam Network connection header.
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

#include "logjam/Network_socket.h"

namespace logjam
{
    //! Object representing a network connection.
    class Network_connection
    {
    public:
        explicit Network_connection(int socket) :
                socket_(socket),
                streambuf_(new logjam::Network_socket(socket_), 8194, 8194),
                stream_(&streambuf_) {}
        Network_connection(const Network_connection& orig) = delete;
        Network_connection(Network_connection&& orig) = delete;
        Network_connection& operator=(const Network_connection& orig) = delete;
        Network_connection& operator=(Network_connection&& orig) = delete;
        virtual ~Network_connection()
        {
            //socket_ is managed by streambuf_.
        }

        virtual const int socket() const
        {
            return socket_;
        }

        virtual lj::Streambuf_bsd<Network_socket>& rdbuf()
        {
            return streambuf_;
        }

        virtual std::iostream& stream()
        {
            return stream_;
        }
    private:
        int socket_;
        lj::Streambuf_bsd<Network_socket> streambuf_;
        std::iostream stream_;
    }; // class logjam::Network_connection
}; // namespace logjam
