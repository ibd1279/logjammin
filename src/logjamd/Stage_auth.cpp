/*!
 \file Stage_auth.cpp
 \brief Logjam server client authentication implementation.
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

#include "logjamd/Stage_auth.h"

#include "logjamd/Auth.h"
#include "logjamd/Connection.h"
#include "logjamd/Stage_execute.h"
#include "logjamd/constants.h"
#include "lj/Bson.h"
#include "lj/Log.h"
#include "lj/Uuid.h"

#include <memory>

namespace
{
    const std::string k_unknown_auth_provider("Unknown auth provider.");
    const std::string k_unknown_auth_method("Unknown auth method.");
    const std::string k_failed_auth_method("Authentication failed.");
    const std::string k_succeeded_auth_method("Authentication succeeded");
    const std::string k_keys_ignored("Authentication succeeded, but ignoring keys on an insecure connection.");
    const std::string k_keys_warning("Authentication succeeded, setting up keys on an insecure channel.");
};

namespace logjamd
{
    Stage_auth::Stage_auth(logjamd::Connection* connection)
            : logjamd::Stage(connection), attempts_(0)
    {
    }

    Stage_auth::~Stage_auth()
    {
    }

    Stage* Stage_auth::logic()
    {
        attempts_++;
        lj::bson::Node n;
        conn()->io() >> n;
        lj::Uuid method_id(lj::bson::as_uuid(n.nav("method")));
        lj::Uuid provider_id(lj::bson::as_uuid(n.nav("provider")));

        lj::bson::Node response;
        response.set_child("stage", lj::bson::new_string(name()));
        response.set_child("success", lj::bson::new_boolean(false));

        log("Attempting Authentication.").end();
        Auth_provider* provider = Auth_registry::provider(provider_id);
        if (provider)
        {
            Auth_method* method = provider->method(method_id);
            if (method)
            {
                User* user = method->authenticate(n.nav("data"));
                if (user)
                {
                    lj::log::out<lj::Info>(k_succeeded_auth_method);
                    response.set_child("success", lj::bson::new_boolean(true));
                    response.set_child("message", lj::bson::new_string(k_succeeded_auth_method));
                    conn()->user(user);
                }
                else
                {
                    lj::log::out<lj::Info>(k_failed_auth_method);
                    response.set_child("message", lj::bson::new_string(k_failed_auth_method));
                }
            }
            else
            {
                lj::log::out<lj::Info>(k_unknown_auth_method);
                response.set_child("message", lj::bson::new_string(k_unknown_auth_method));
            }
        }
        else
        {
            lj::log::out<lj::Info>(k_unknown_auth_provider);
            response.set_child("message",
                    lj::bson::new_string(k_unknown_auth_provider));
        }

        Stage* next_stage = nullptr;
        if (conn()->user())
        {
            // TODO impersonation

            // key setup.
            if (n.exists("keys"))
            {
                lj::bson::Node& keys = n["keys"];
                bool force_keys = lj::bson::as_boolean(
                        n["i_know_connection_is_insecure"]);
                if (conn()->secure() || force_keys)
                {
                    // Notify the user if being forced to use keys.
                    if (force_keys)
                    {
                        response.set_child("message",
                                lj::bson::new_string(k_keys_warning));
                    }

                    for (auto iter = keys.to_vector().begin();
                            keys.to_vector().end() != iter;
                            ++iter)
                    {
                        std::string name(lj::bson::as_string(
                                (*iter)->nav("name")));
                        uint32_t sz;
                        lj::bson::Binary_type bt;
                        const uint8_t* data = lj::bson::as_binary(
                                (*iter)->nav("data"),
                                &bt,
                                &sz);
                        conn()->set_crypto_key(name, data, sz);
                    }
                }
                else
                {
                    response.set_child("message",
                            lj::bson::new_string(k_keys_ignored));
                }
            }

            next_stage = new Stage_execute(conn());
        }
        else
        {
            if (attempts_ < 3)
            {
                next_stage = this;
            }
            else
            {
                next_stage = NULL;
            }
        }

        conn()->io() << response;
        return next_stage;
    }
    std::string Stage_auth::name()
    {
        return std::string("Authentication");
    }
};

