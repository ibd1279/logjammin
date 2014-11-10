/*!
 \file Stage_auth.cpp
 \brief Logjam server client authentication implementation.
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

#include "logjamd/Stage_auth.h"

#include "logjamd/Response.h"
#include "logjamd/Stage_execute.h"
#include "logjamd/constants.h"
#include "logjam/User.h"
#include "lj/Bson.h"
#include "lj/Log.h"
#include "lj/Uuid.h"

#include <memory>

namespace
{
    const int64_t k_max_auth_attempts(3);
    const std::string k_exceeded_max_attempts("You have exceeded the maximum allowed number of auth attempts.");
    const std::string k_unknown_auth_provider("Unknown auth provider.");
    const std::string k_unknown_auth_method("Unknown auth method.");
    const std::string k_failed_auth_method("Authentication failed.");
    const std::string k_succeeded_auth_method("Authentication succeeded");
    const std::string k_keys_ignored("Authentication succeeded, but ignoring keys on an insecure connection.");
    const std::string k_keys_warning("Authentication succeeded, setting up keys on an insecure channel.");
};

namespace logjamd
{
    std::unique_ptr<logjam::Stage> Stage_auth::logic(logjam::pool::Swimmer& swmr) const
    {
        // abort if we have attempted to auth too many times.
        lj::bson::Node& attempts =
                swmr.context().node().nav("auth/attempts");
        lj::bson::increment(attempts, 1);
        if (k_max_auth_attempts <= lj::bson::as_int64(attempts))
        {
            swmr.io() << response::new_error(*this,
                    k_exceeded_max_attempts);
            return std::unique_ptr<Stage>(nullptr);
        }

        // Get the input data.
        lj::bson::Node n;
        swmr.io() >> n;
        std::string method_name(lj::bson::as_string(n["method"]));
        std::string provider_name(lj::bson::as_string(n["provider"]));

        // prepare the response
        lj::bson::Node response(response::new_empty(*this));
        response.set_child("success", lj::bson::new_boolean(false));

        log("Looking up method %s in provider %s.")
                << method_name
                << provider_name
                << lj::log::end;
        try
        {
            logjam::Authentication_repository& auth_repo =
                    swmr.context().environs().authentication_repository();
            logjam::Authentication_provider& provider =
                    auth_repo.provider(provider_name);
            logjam::Authentication_method& method =
                    provider.method(method_name);
            lj::Uuid user_id(method.authenticate(n["data"]));

            logjam::User_repository& user_repo =
                    swmr.context().environs().user_repository();
            logjam::User user(user_repo.find(user_id));
            lj::log::format<lj::Info>("Authentication succeeded for %s.")
                    << user.name()
                    << lj::log::end;

            response.set_child("success", lj::bson::new_boolean(true));
            response.set_child("message", lj::bson::new_string(k_succeeded_auth_method));
            
            // update the ctx object for the right user.
            swmr.context().user() = user;
        }
        catch (logjam::Authentication_provider_not_found_exception& ex)
        {
            log("Failed to find provider %s.")
                    << provider_name
                    << lj::log::end;
            response.set_child("message", lj::bson::new_string(k_unknown_auth_provider));
        }
        catch (logjam::Authentication_method_not_found_exception& ex)
        {
            log("Failed to find method %s in provider %s.")
                    << method_name
                    << provider_name
                    << lj::log::end;
            response.set_child("message", lj::bson::new_string(k_unknown_auth_method));
        }
        catch (logjam::User_not_found_exception& ex)
        {
            response.set_child("message", lj::bson::new_string(k_failed_auth_method));
        }

        // Send the response.
        swmr.io() << response;

        // selet the next stage.
        std::unique_ptr<Stage> next_stage;
        if (lj::bson::as_boolean(response["success"]))
        {
            // TODO impersonation

            // Move on to the execution stage.
            next_stage.reset(new Stage_execute());
        }
        else
        {
            next_stage = this->clone();
        }

        return next_stage;
    }

    std::string Stage_auth::name() const
    {
        return std::string("Authentication");
    }

    std::unique_ptr<logjam::Stage> Stage_auth::clone() const
    {
        return std::unique_ptr<Stage>(new Stage_auth());
    }
};

