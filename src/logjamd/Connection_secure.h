#pragma once
/*!
 \file logjamd/Connection_secure.h
 \brief Logjam server connection to a client with support for secure communications.
 \author Jason Watson

 Copyright (c) 2010, Jason Watson
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

#include "logjamd/Connection.h"
#include "logjam/Network_connection.h"
#include "lj/Thread.h"
#include "lj/Wiper.h"
#include <map>
#include <memory>
#include <string>
#include <thread>

namespace logjamd
{
    class Server_secure;

    //! A Connection implementation that supports security.
    /*!
     \par
     This connection implementation is for the Server_secure implementation.
     It depends on OpenSSL for TLS support. The secure method will only
     return true if the connection has established TLS.
     \par
     This connection type depends on lj::Thread. As such it also implements
     the \c lj::Work interface.
     \sa logjamd::Server_secure
     \sa lj::Thread
     \sa lj::Work
     */
    class Connection_secure :
            public logjamd::Connection, lj::Work
    {
    public:
        //! Create a secure connection object.
        /*!
         \par Resource Management
         This object is responsible for releasing the memory associated with
         \c state Server is not released by this class. \c socket_desc is
         released by this class.
         \param server The server associated with this connection.
         \param state The state associated with this server.
         \param connection The network connection.
         \param insecure_stream The iostream for initial communication.
         \sa logjamd::Connection::Connection
         */
        Connection_secure(logjamd::Server_secure* server,
                lj::bson::Node* state,
                logjam::Network_connection&& connection,
                std::iostream* insecure_stream);

        //! Destructor
        virtual ~Connection_secure();
        virtual void start() override;
        virtual bool secure() const override
        {
            return secure_;
        }
        virtual bool securable() const override
        {
            return !secure_;
        }
        virtual void make_secure() override;
        virtual void close() override;
        virtual void set_crypto_key(const std::string& identifier,
                const void* key,
                int sz) override;
        virtual const void* get_crypto_key(const std::string& identifier,
                int* sz) override;
        virtual void run() override;
        virtual void cleanup() override;
    protected:
        //! Get the thread associated with this connection.
        /*!
         \return A reference to the Thread.
         */
        inline lj::Thread& thread()
        {
            return *thread_;
        }
    private:
        //! Secure Server for getting crypto sessions.
        /*!
         \par Resource Management
         This object does not own this memory.
         */
        logjamd::Server_secure* server_; //!< Secure Server for getting crypto session.
        logjam::Network_connection connection_; //!< Wrapper for the socket.
        lj::Thread* thread_; //!< Thread to process requests.
        bool secure_; //!< Flag indicating the security of the link.
        std::map<std::string, std::unique_ptr<uint8_t[], lj::Wiper<uint8_t[]> > > keys_; //!< Crypto key management.
    };
};


