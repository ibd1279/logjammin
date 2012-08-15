/*!
 \file Stage_http_adapt.cpp
 \brief Logjam server stage for converting telnet json input into bson input.
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

#include "lj/Log.h"
#include "logjamd/Stage_http_adapt.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/constants.h"

namespace
{
};

namespace logjamd
{
    Stage_http_adapt::Stage_http_adapt(Connection* connection) :
            Stage(connection),
            pipe_(),
            faux_connection_(connection, new std::iostream(&pipe_)),
            real_stage_(new Stage_auth(&faux_connection_)),
            language_("lua")
    {
    }

    Stage_http_adapt::~Stage_http_adapt()
    {
        if (real_stage_)
        {
            delete real_stage_;
            real_stage_ = nullptr;
        }
    }

    Stage* Stage_http_adapt::logic()
    {
        // default next-stage is to disconnect.
        Stage* next_stage = nullptr;

        // If the conection is insecure, use default login.
        log("Using insecure adapter authentication.").end();

        lj::bson::Node auth_request;
        auth_request.set_child("method",
                lj::bson::new_uuid(lj::Uuid(k_auth_method,
                        "password_hash",
                        13)));
        auth_request.set_child("provider",
                lj::bson::new_uuid(lj::Uuid(k_auth_provider,
                        "local",
                        5)));
        auth_request.set_child("data/login",
                lj::bson::new_string(k_user_login_json));
        auth_request.set_child("data/password",
                lj::bson::new_string(k_user_password_json));

        // Login as the fake, limited user.
        pipe_.sink() << auth_request;
        next_stage = real_stage_->logic();
        lj::bson::Node auth_response;
        pipe_.source() >> auth_response;

        // If the login was successful
        if (next_stage != nullptr || faux_connection().user() != nullptr)
        {
            // Immediately goto the next stage.
            if (real_stage_ != next_stage)
            {
                delete real_stage_;
                real_stage_ = next_stage;
            }
            next_stage = nullptr;

            // TODO handle multi-line commands.
            std::string cmd;
            std::getline(conn()->io(), cmd);
            if (!conn()->io().good())
            {
                // Handle a read error.
                lj::log::out<lj::Warning>("Some kind of read error.");
                next_stage = nullptr;
            }
            else
            {
                // trim off the HTTP version at the end.
                if (cmd.size() > 10)
                {
                    // cmd is long enough to have an http version.
                    cmd = cmd.erase(cmd.size() - 10);
                }
                else 
                {
                    // cmd is not long enough to have an http version,
                    // so erase everything.
                    cmd.erase();
                }
                log("Using [%s] for the command.").end(cmd);

                // Remove all the other headers because we are ignoring them
                // right now.
                std::string discard;
                do
                {
                    std::getline(conn()->io(), discard);

                    // remove the trailing \r for logging reasons.
                    if (discard.size() > 0)
                    {
                        discard.erase(discard.size() -1);
                    }
                    log("Discarded HTTP header [%s].").end(discard);
                }
                while(conn()->io().good() && discard.compare("") != 0);

                // Create the bson request.
                lj::bson::Node request;
                request.set_child("command",
                        lj::bson::new_string(cmd));
                request.set_child("language",
                        lj::bson::new_string(language_));
                pipe_.sink() << request;

                // process the bson request.
                next_stage = real_stage_->logic();

                // http is only allowed to do one request per connection.
                if (next_stage != nullptr && next_stage != real_stage_)
                {
                    delete next_stage;
                }
                next_stage = nullptr;

                // convert the response into json.
                lj::bson::Node response;
                pipe_.source() >> response;

                // This should be updated to deal with exceptions, etc.
                conn()->io() << "HTTP/1.0 200 OK\r\nContent-Type: text/plain; charset=\"UTF-8\"\r\n\r\n";
                conn()->io() << lj::bson::as_pretty_json(response)
                        << std::endl;;
                conn()->io().flush();
            }
        }
        else
        {
            conn()->io() << "HTTP/1.0 403 Forbidden\r\nContent-Type: text/plain; charset=\"UTF-8\"\r\n\r\n";
            conn()->io() << lj::bson::as_pretty_json(auth_response) << std::endl;
        }

        // Clean up next_stage and real stage.
        if (next_stage)
        {
            if (next_stage != real_stage_)
            {
                delete real_stage_;
                real_stage_ = next_stage;
                // real_stage_ gets cleaned up in the destructor.
            }
        }

        // All http disconnections immediately disconnect.
        log("Disconnecting.").end();
        real_stage_ = nullptr;
        return nullptr;
    }

    std::string Stage_http_adapt::name()
    {
        if (real_stage_)
        {
            return real_stage_->name();
        }
        return std::string("JSON-Adapter");
    }
};

