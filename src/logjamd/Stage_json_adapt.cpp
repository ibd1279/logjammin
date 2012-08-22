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

namespace
{
};

namespace logjamd
{
    Stage_json_adapt::Stage_json_adapt(Connection* connection) :
            Stage_adapt(connection),
            real_stage_(new_auth_stage())
    {
    }

    Stage_json_adapt::~Stage_json_adapt()
    {
    }

    Stage* Stage_json_adapt::logic()
    {
        if (conn()->secure() || faux_connection().user() != nullptr)
        {
            // If we already have a user, or the connection is secure.

            // TODO handle multi-line commands.
            std::string cmd;
            std::getline(conn()->io(), cmd);
            if (!conn()->io().good())
            {
                // Handle a read error.
                log("Some kind of read error.").end();
                return nullptr;
            }
            else
            {
                // Create the request.
                lj::bson::Node request;
                request.set_child("command",
                        lj::bson::new_string(cmd));
                request.set_child("language",
                        lj::bson::new_string(language()));

                // process the request.
                pipe().sink() << request;
                std::unique_ptr<Stage> next_stage(real_stage_->logic());

                // Deal with any stages that return themselves. unique_ptr does
                // not guard against self resets.
                if (real_stage_ == next_stage)
                {  
                    next_stage.release();
                }
                else
                {  
                    real_stage_.reset(next_stage.release());
                }

                // convert the response into json.
                lj::bson::Node response;
                pipe().source() >> response;

                // Switch the command languages if necessary.
                if (response.exists("next_language"))
                {
                    std::string lang =
                            lj::bson::as_string(response["next_language"]);
                    set_language(lang);
                    response.set_child("next_language", NULL);
                }

                // Prepare to disconnect.
                if (response.exists("disconnect") &&
                        lj::bson::as_boolean(response["disconnect"]))
                {
                    response.set_child("disconnect", NULL);
                    real_stage_.reset(nullptr);
                }

                // Communicate the response.
                conn()->io() << lj::bson::as_pretty_json(response)
                        << std::endl;;
                conn()->io().flush();
            }
        }
        else
        {
            // If the conection is insecure and there is no user,
            // use the default login.
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

            // Execute the authentication.
            pipe().sink() << auth_request;
            std::unique_ptr<Stage> next_stage(real_stage_->logic());
            
            // Deal with any stages that return themselves. unique_ptr does
            // not guard against self resets.
            if (real_stage_ == next_stage)
            {  
                next_stage.release();
            }
            else
            {  
                real_stage_.reset(next_stage.release());
            }

            // convert the response into json.
            lj::bson::Node auth_response;
            pipe().source() >> auth_response;

            // communicate the response.
            conn()->io() << lj::bson::as_pretty_json(auth_response) << std::endl;
            conn()->io().flush();
        }

        if (real_stage_)
        {
            return this;
        }
        else
        {
            log("Disconnecting.").end();
            return nullptr;
        }
    }

    std::string Stage_json_adapt::name()
    {
        std::string my_name("JSON-Adapter-");
        return my_name + real_stage_->name();
    }
};
