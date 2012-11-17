#pragma once
/*
 \file logjam/Network_connection.h
 \brief Logjam Network connection interface.
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

namespace logjam
{
    //! Object representing a network connection.
    class Network_connection
    {
    public:
        //! Create a new network connection object.
        Network_connection();
        
        //! Create a new network connection object around an existing socket.
        /*!
         The socket is expected to be open and ready for communication.
         \param socket An existing socket descriptor.
         */
        explicit Network_connection(int socket);
        
        //! Deleted copy constructor
        /*!
         \param orig The original object.
         */
        Network_connection(const Network_connection& orig) = delete;
        
        //! Move constructor
        /*!
         \param orig The original object.
         */
        Network_connection(Network_connection&& orig);
        
        //! Destructor
        /*!
         If the socket is open, it is immediately closed.
         */
        virtual ~Network_connection();
        
        //! Deleted copy operator
        /*!
         \param orig The original object.
         \return This object.
         */
        Network_connection& operator=(const Network_connection& orig) = delete;
        
        //! Move operator
        /*!
         \param orig The original object.
         \return This object.
         */
        Network_connection& operator=(Network_connection&& orig);
        
        //! Connect to a target address.
        /*!
         \note Remote Information.
         This object does not retain any information about who it is connected
         to.
         \param target to connect to.
         \throws lj::Exception if the connection could not be established.
         */
        void connect(const struct addrinfo& target);
        
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
        int socket_;
    }; // class logjam::Network_connection
}; // namespace logjam