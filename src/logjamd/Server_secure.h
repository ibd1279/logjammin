#pragma once
/*!
 \file logjamd/Server_secure.h
 \brief Logjam server networking header.
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

#include "logjamd/Server.h"
#include "logjam/Tls_credentials.h"
#include "logjam/Tls_session.h"
#include <list>
#include <map>
#include <memory>

// Forward declarations.
namespace lj
{
    class Thread;
}

namespace logjamd
{
    class Connection_secure;

    //! An implementation of the logjamd::Server that supports TLS.
    class Server_secure : public logjamd::Server
    {
    public:
        typedef logjam::Tls_session<logjam::Tls_credentials_reuse_adapter<logjam::Tls_credentials_anonymous_server>> Session;
        typedef std::map<std::string, std::iostream*> Peer_map;

        Server_secure(lj::bson::Node* config);
        Server_secure(const Server_secure& o) = delete;
        Server_secure& operator=(const Server_secure& o) = delete;
        virtual ~Server_secure();
        virtual void startup() override;
        virtual void listen() override;
        virtual void shutdown() override;
        virtual void detach(Connection* conn) override;

        //! Get a session associated with this server.
        /*!
         The returned session object is fully setup and ready for communication.
         \param socket_descriptor The socket associated with the session communication.
         \return A new session.
         */
        virtual std::unique_ptr<Session> new_session(int socket_descriptor);
        
        virtual Peer_map& peers();
    private:
        int io_;
        bool running_;
        std::list<logjamd::Connection_secure*> connections_;
        Peer_map peers_;
        lj::Thread* peers_thread_;
        logjam::Tls_credentials_anonymous_server credentials_;
        logjam::Tls_key_exchange_diffie_hellman key_exchange_;
    };
};
