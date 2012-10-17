/*!
 \file logjamd.cpp
 \brief Logjam Server Executable
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

#include "logjamd/Auth.h"
#include "logjamd/Auth_local.h"
#include "logjamd/Server.h"
#include "logjamd/Server_secure.h"
#include "logjamd/constants.h"
#include "logjam/Tls_globals.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

void setup_credentials(
        logjamd::Auth_provider* auth,
        const lj::Uuid& id,
        const std::string& login,
        const std::string& pword)
{
    logjamd::User u(id, login);
    lj::bson::Node n;
    n.set_child("login",
            lj::bson::new_string(login));
    n.set_child("password",
            lj::bson::new_string(pword));
    const lj::Uuid k_auth_method_password_hash(logjamd::k_auth_method,
            "password_hash",
            13);
    auth->method(k_auth_method_password_hash)->change_credentials(&u, &u, n);
}

//! Server main entry point.
int main(int argc, char* const argv[]) {
    lj::bson::Node* config = new lj::bson::Node();
    lj::Uuid sid;
    config->set_child("server/listen", lj::bson::new_string("*:12345"));
    config->set_child("server/cluster", lj::bson::new_array());
    config->push_child("server/cluster", lj::bson::new_string("localhost:12345"));
    config->push_child("server/cluster", lj::bson::new_string("localhost:12346"));

    // TODO This is completely in the wrong place, but it has to be here
    // to make the telnet stuff work during development right now.
    lj::log::out<lj::Info>("Adding the Local auth provider.");
    logjamd::Auth_provider_local* local_auth =
            new logjamd::Auth_provider_local();
    logjamd::Auth_registry::enable(local_auth);

    lj::log::out<lj::Info>("Creating read-only accounts.");
    setup_credentials(local_auth,
            logjamd::k_user_id_json,
            logjamd::k_user_login_json,
            logjamd::k_user_password_json);
    setup_credentials(local_auth,
            logjamd::k_user_id_http,
            logjamd::k_user_login_http,
            logjamd::k_user_password_http);

    // Run the server.
    logjam::Tls_globals tls_globals;
    logjamd::Server* server = new logjamd::Server_secure(config);

    try
    {
        server->startup();
        server->listen();
        server->shutdown();
    }
    catch (lj::Exception& ex)
    {
        lj::log::format<lj::Critical>("Exiting: %s").end(ex);
    }

    return 0;
}
