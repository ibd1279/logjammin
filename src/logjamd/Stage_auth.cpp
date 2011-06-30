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
#include "logjamd/Connection.h"

#include "lj/Bson.h"
#include "lj/Log.h"
#include "lj/Uuid.h"

#include <memory>

namespace
{
    const lj::Uuid k_logjamd(lj::Uuid::k_nil, "logjamd", 7);
    const lj::Uuid k_auth_method(k_logjamd, "auth_method", 11);
    const lj::Uuid k_auth_method_fake(k_auth_method, "fake", 4);

    const lj::Uuid k_auth_provider(k_logjamd, "auth_provider", 13);
    const lj::Uuid k_auth_provider_local(k_auth_provider, "local", 5);
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
        lj::Uuid method(lj::bson::as_uuid(n.nav("method")));
        lj::Uuid provider(lj::bson::as_uuid(n.nav("provider")));
        // identity
        // token

        lj::Log::info("Login request for %s/%s.")
                << method
                << provider
                << lj::Log::end;

        lj::bson::Node response;
        response.set_child("stage", lj::bson::new_string(name()));
        response.set_child("success", lj::bson::new_boolean(false));

        if (k_auth_method_fake == method)
        {
            if (k_auth_provider_local == provider)
            {
                lj::Log::info.log("Performing Fake/Local authentication.");
                response.set_child("success", lj::bson::new_boolean(true));
                conn()->io() << response;

                return NULL;
            }
        }

        lj::Log::info.log("Unknown auth type.");
        conn()->io() << response;

        return NULL;
    }
    std::string Stage_auth::name()
    {
        return std::string("Authentication");
    }
};

