/*
 \file logjam.cpp
 \author Jason Watson

 Copyright (c) 2009, Jason Watson
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


#include "logjam/Network_address_info.h"
#include "logjam/Tls_credentials.h"
#include "logjam/Tls_globals.h"
#include "logjam/Tls_session.h"
#include "lj/Bson.h"
#include "lj/Log.h"
#include "lj/Streambuf_bsd.h"
#include "logjamd/constants.h"

#include <iostream>
#include <unistd.h>


int main(int argc, char * const argv[])
{
    try
    {
        logjam::Tls_globals globals; // this ensures that the deinit is called when the stack unwinds.

        logjam::Tls_session<logjam::Tls_credentials_anonymous_client>* session =
                new logjam::Tls_session<logjam::Tls_credentials_anonymous_client>(GNUTLS_CLIENT);

        std::string target_hostname("localhost");
        session->set_user_data(&target_hostname);
        session->set_hostname(target_hostname);
        session->set_cipher_priority("NORMAL:+ANON-ECDH:+ANON-DH");

        logjam::Network_address_info info(target_hostname,
                "12345",
                0,
                AF_UNSPEC,
                SOCK_STREAM,
                0);

        int sockfd = -1;
        while (info.next() && 0 > sockfd)
        {
            sockfd = socket(info.current().ai_family,
                    info.current().ai_socktype,
                    info.current().ai_protocol);
            if (0 > sockfd)
            {
                lj::log::format<lj::Warning>("Unable to create the socket. [%s]")
                        << strerror(errno)
                        << lj::log::end;
                continue;
            }

            int result = connect(sockfd,
                    info.current().ai_addr,
                    info.current().ai_addrlen);
            if (0 > result)
            {
                ::close(sockfd);
                sockfd = -1;
                lj::log::format<lj::Warning>("Unable to connect. [%s]")
                        << strerror(errno)
                        << lj::log::end;
                continue;
            }
        }

        if (0 > sockfd)
        {
            lj::log::out<lj::Critical>("Unable to connect to host.");
            return 1;
        }

        lj::Streambuf_bsd<lj::medium::Socket>* plain_buffer =
                new lj::Streambuf_bsd<lj::medium::Socket>(new lj::medium::Socket(sockfd), 8192, 8192);
        std::iostream io(plain_buffer);

        lj::log::out<lj::Info>("requesting TLS.");

        lj::bson::Node n;
        io << "+tls\n";
        io.flush();
        io >> n;
        if (lj::bson::as_boolean(n.nav("/success")))
        {
            session->set_socket(sockfd);
            lj::Streambuf_bsd<logjam::Tls_session<logjam::Tls_credentials_anonymous_client> >* crypt_buffer =
                    new lj::Streambuf_bsd<logjam::Tls_session<logjam::Tls_credentials_anonymous_client> >(session, 8192, 8192);
            lj::log::out<lj::Info>("Starting handshake");
            session->handshake();
            lj::log::out<lj::Info>("Completed handshake");
            io.rdbuf(crypt_buffer);
        }
        else
        {
            lj::log::out<lj::Info>("Server doesn't support TLS.");
            return 1;
        }

        io >> n;
        if (lj::bson::as_boolean(n.nav("/success")))
        {
            lj::log::out<lj::Info>("We are now secure.");
        }
        io << "bson\n";
        lj::log::out<lj::Info>("sent bson hello.");
        io.flush();

        io >> n;
        if (lj::bson::as_boolean(n.nav("/success")))
        {
            lj::log::out<lj::Info>("Now in authentication.");
        }

        lj::bson::Node auth;
        auth.set_child("/method", lj::bson::new_uuid(logjamd::k_auth_method_password));
        auth.set_child("/provider", lj::bson::new_uuid(logjamd::k_auth_provider_local));
        auth.set_child("/data/login", lj::bson::new_string(logjamd::k_user_login_json));
        auth.set_child("/data/password", lj::bson::new_string(logjamd::k_user_password_json));
        io << auth;
        io.flush();
        io >> n;
        if (lj::bson::as_boolean(n.nav("/success")))
        {
            lj::log::out<lj::Info>("Authenticated.");
        }
    }
    catch (const lj::Exception& ex)
    {
        lj::log::out<lj::Error>(ex.str());
    }
}