/*!
 \file Stage_json_adapt.cpp
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
#include "logjamd/Stage_json_adapt.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/constants.h"

namespace logjamd
{
    Stage_json_adapt::Stage_json_adapt(Connection* connection) :
            Stage(connection),
            pipe_(),
            faux_connection_(connection, new std::iostream(&pipe_)),
            real_stage_(new Stage_auth(&faux_connection_))
    {
    }

    Stage_json_adapt::~Stage_json_adapt()
    {
        if (real_stage_)
        {
            delete real_stage_;
            real_stage_ = NULL;
        }
    }

    Stage* Stage_json_adapt::logic()
    {
        Stage* next_stage = NULL;
        if (conn()->secure() || conn()->user() != NULL)
        {
            std::string cmd;
            if (!std::getline(conn()->io(), cmd).good())
            {
                // Handle a read error.
            }

            std::unique_ptr<lj::bson::Node> request(lj::bson::parse_string(cmd));
            lj::bson::Node response;
            pipe_.sink() << *request;

            next_stage = real_stage_->logic();

            pipe_.source() >> response;
            conn()->io() << response;
        }
        else
        {
            // If the conection is insecure, use default login.
            log("Using insecure adapter authentication.") << lj::Log::end;

            lj::bson::Node auth_request;
            lj::bson::Node auth_response;
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

            pipe_.sink() << auth_request;

            next_stage = real_stage_->logic();

            pipe_.source() >> auth_response;
            conn()->io() << lj::bson::as_string(auth_response);
        }

        if (next_stage)
        {
            log("Next stage is %s.") << next_stage->name() << lj::Log::end;
            if (next_stage != real_stage_)
            {
                delete real_stage_;
                real_stage_ = next_stage;
            }
            return this;
        }
        else
        {
            log("Disconnecting.") << lj::Log::end;
            real_stage_ = NULL;
            return NULL;
        }
    }

    std::string Stage_json_adapt::name()
    {
        if (real_stage_)
        {
            return real_stage_->name();
        }
        return std::string("JSON-Adapter");
    }
};

