#pragma once
/*
 \file logjam/Network_socket.h
 \brief Logjam Network socket header.
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

#include "lj/streambuf_bsd.h"

extern "C"
{
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
}

namespace logjam
{
    //! Object representing a network connection.
    class Network_socket : public lj::medium::Socket
    {
    public:
        //! Create a new network connection object.
        Network_socket();
        
        //! Create a new network connection object around an existing socket.
        /*!
         The socket is expected to be open and ready for communication.
         \param socket An existing socket descriptor.
         */
        explicit Network_socket(int socket);
        
        //! Deleted copy constructor
        /*!
         \param orig The original object.
         */
        Network_socket(const Network_socket& orig) = delete;
        
        //! Move constructor
        /*!
         \param orig The original object.
         */
        Network_socket(Network_socket&& orig);
        
        //! Deleted copy operator
        /*!
         \param orig The original object.
         \return This object.
         */
        Network_socket& operator=(const Network_socket& orig) = delete;
        
        //! Move operator
        /*!
         \param orig The original object.
         \return This object.
         */
        Network_socket& operator=(Network_socket&& orig);
        
        //! Destructor
        /*!
         If the socket is open, it is immediately closed.
         */
        virtual ~Network_socket();
        
        //! Close an open socket.
        /*!
         No action is performed if the socket is not open.
         */
        void close();
        
        //! Get the socket file descriptor.
        /*!
         \return the socket descriptor.
         */
        int socket() const;

        //! Check if the socket is currently open.
        /*!
         \return True if the connection is expected to be open. false otherwise.
         */
        inline bool is_open() const
        {
            return is_open_;
        }

    private:
        bool is_open_;
    }; // class logjam::Network_socket

    //! Connect to a target address.
    /*!
     \note Remote Information.
     The \c Network_socket object does not retain any information about
     the target.
     \param target to connect to.
     \return An open socket.
     \throws lj::Exception if the connection could not be established.
     */
    Network_socket socket_for_target(const struct addrinfo& target);
}; // namespace logjam
