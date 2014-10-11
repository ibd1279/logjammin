/*
 \file logjam/Client_socket.cpp
 \brief Logjam helper class for opening a new TLS connection.
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

#include "logjam/Client_socket.h"
#include "logjam/Network_address_info.h"
#include "logjam/Network_socket.h"
#include "logjam/Tls_credentials.h"
#include "logjam/Tls_session.h"
#include "lj/Bson.h"
#include "lj/Exception.h"
#include "lj/Log.h"
#include "lj/Streambuf_bsd.h"

namespace
{
    template<class credT>
    class iostream_secure : public std::iostream
    {
    public:
        iostream_secure(logjam::Network_socket&& conn,
                logjam::Tls_session<credT>* sess,
                lj::Streambuf_bsd<logjam::Tls_session<credT>>* buf) :
                std::iostream(buf),
                connection_(std::move(conn)),
                session_(sess),
                buffer_(buf)
                
        {
        }
        virtual ~iostream_secure()
        {
            delete session_;
            delete buffer_;
        }
    private:
        logjam::Network_socket connection_;
        logjam::Tls_session<credT>* session_;
        lj::Streambuf_bsd<logjam::Tls_session<credT>>* buffer_;
    };
}; // namespace (anonymous)

namespace logjam
{
    namespace client
    {
        bool is_success(const lj::bson::Node& response)
        {
            const lj::bson::Node* success = response.path("/success");
            if (success == nullptr)
            {
                return false;
            }
            
            return lj::bson::as_boolean(*success);
        }
        
        std::string message(const lj::bson::Node& response)
        {
            const lj::bson::Node* message = response.path("/message");
            if (message == nullptr)
            {
                return "";
            }
            
            return std::move(lj::bson::as_string(*message));
        }

        std::iostream* create_connection(const std::string& target_host,
                const std::string& target_mode)
        {
            logjam::Tls_session<logjam::Tls_credentials_anonymous_client>* session =
                    new logjam::Tls_session<logjam::Tls_credentials_anonymous_client>(GNUTLS_CLIENT);

            session->set_cipher_priority("NORMAL:+ANON-ECDH:+ANON-DH");
            
            logjam::Network_address_info info(target_host,
                    0,
                    AF_UNSPEC,
                    SOCK_STREAM,
                    0);

            logjam::Network_socket connection;
            while (info.next() && !connection.is_open())
            {
                try
                {
                    connection = logjam::socket_for_target(info.current());
                }
                catch (lj::Exception ex)
                {
                    lj::log::format<lj::Critical>("%s").end(ex);
                }
            }

            if (!connection.is_open())
            {
                throw LJ__Exception("Unable to connect to host.");
            }
            
            lj::log::out<lj::Info>("Connection established. Requesting TLS.");
            
            lj::Streambuf_bsd<lj::medium::Socket>* plain_buffer =
                    new lj::Streambuf_bsd<lj::medium::Socket>(new lj::medium::Socket(connection.socket()), 8192, 8192);
            std::iostream io(plain_buffer);

            lj::bson::Node response;
            io << "+tls\n";
            io.flush();
            io >> response;

            // Did the server accept the +tls command.
            iostream_secure<logjam::Tls_credentials_anonymous_client>* sec_io;
            if (is_success(response))
            {
                session->set_socket(connection.socket());
                lj::Streambuf_bsd<logjam::Tls_session<logjam::Tls_credentials_anonymous_client> >* crypt_buffer =
                        new lj::Streambuf_bsd<logjam::Tls_session<logjam::Tls_credentials_anonymous_client> >(session, 8192, 8192);
                lj::log::out<lj::Debug>("Starting handshake");
                session->handshake();
                lj::log::out<lj::Debug>("Completed handshake");
                sec_io = new iostream_secure<logjam::Tls_credentials_anonymous_client>(std::move(connection),
                        session,
                        crypt_buffer);
            }
            else
            {
                throw LJ__Exception("Server does not support TLS.");
            }
            
            // sec_io should not be null at this point.
            assert(sec_io);
            
            // After the server completes the session handshake, it should have
            // sent another bson object over TLS with the "success" flag set
            // to true.
            (*sec_io) >> response;
            
            // At this point, we replace 
            if (is_success(response))
            {
                lj::log::out<lj::Debug>("We are now secure.");
            }
            else
            {
                delete sec_io;
                throw LJ__Exception("Could not establish a secure connection to the server.");
            }
            
            // Since we go back to the pre stage after switching TLS, we need to
            // do the general pre auth.
            (*sec_io) << target_mode << "\n";
            sec_io->flush();
            (*sec_io) >> response;
            if (is_success(response))
            {
                lj::log::out<lj::Debug>("Now in authentication.");
            }
            else
            {
                std::ostringstream oss;
                oss << "Could not switch to mode " << target_mode << ".";
                delete sec_io;
                throw LJ__Exception(oss.str());
            }
            
            return sec_io;
        }
    }; // namespace logjam::client
}; // namespace logjam
