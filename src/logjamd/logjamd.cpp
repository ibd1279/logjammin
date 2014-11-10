/*!
 \file logjamd.cpp
 \brief Logjam Server Executable
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

#include "lj/Args.h"
#include "logjamd/Auth_local.h"
#include "logjamd/Pool_listen_threads.h"
#include "logjamd/constants.h"
#include "logjam/User.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>

static void setup_credentials(
        logjam::Authentication_method& method,
        logjam::User_repository& user_repo,
        const lj::Uuid& id,
        const std::string& login,
        const std::string& pword)
{
    logjam::User u(id, login);
    user_repo.store(u);

    lj::bson::Node n;
    n.set_child("login",
            lj::bson::new_string(login));
    n.set_child("password",
            lj::bson::new_string(pword));
    method.change_credential(id, n);
}

//! Server main entry point.
int main(int argc, char* const argv[]) {
    lj::Arg_parser arg_parser(argc, argv);

    // TODO Move settings configuration to a function.
    lj::Setting_arg config_file_setting(arg_parser, "-c", "--config", "Location of the configuration file.", "");

    // Load the configuration values.
    std::unique_ptr<lj::bson::Node> config;
    try
    {
        arg_parser.parse();

        if (!config_file_setting.present())
        {
            throw lj::Exception("logjamd", "No configuration file specified.");
        }

        std::ifstream config_file_stream(config_file_setting.str());
        if (!config_file_stream.good())
        {
            throw lj::Exception("logjamd", config_file_setting.str() + " could not be opened.");
        }

        config.reset(lj::bson::parse_json(config_file_stream));
    }
    catch (lj::Exception& ex)
    {
        lj::log::format<lj::Critical>("Failure: %s").end(ex);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(1);
    }

    lj::log::out<lj::Info>("Creating Environs.");
    logjam::Authentication_repository auth_repo;
    auth_repo.enable(
            new logjam::Authentication_provider_simple<logjamd::Auth_method_password_hash>(
                    logjamd::k_auth_provider_local));
    logjam::User_repository user_repo;

    lj::log::out<lj::Info>("Creating read-only accounts.");
    logjam::Authentication_method& method =
            auth_repo.provider(logjamd::k_auth_provider_local).method(
                    logjamd::k_auth_method_password);

    setup_credentials(method,
            user_repo,
            logjamd::k_user_id_json,
            logjamd::k_user_login_json,
            logjamd::k_user_password_json);
    setup_credentials(method,
            user_repo,
            logjamd::k_user_id_http,
            logjamd::k_user_login_http,
            logjamd::k_user_password_http);

    // Run the server.
    logjam::Environs environs(std::move(*config), &user_repo, &auth_repo);
    logjamd::pool::Area_listener inbound(std::move(environs));

    try
    {
        inbound.prepare();
        inbound.open();
    }
    catch (lj::Exception& ex)
    {
        lj::log::format<lj::Critical>("Exiting: %s").end(ex);
    }

    return 0;
}
