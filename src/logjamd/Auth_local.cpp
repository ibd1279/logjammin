/*!
 \file logjamd/Auth_local.cpp
 \brief Logjam server authentication implementation.
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
#include "logjamd/Auth.h"
#include "logjamd/Auth_local.h"
#include "logjamd/constants.h"

namespace 
{
    const lj::Uuid password_hash_method(logjamd::k_auth_method, "password_hash", 13);

    lj::Uuid compute_digest(const lj::bson::Node& data)
    {
        // Compute the realm.
        const std::string realm_in(lj::bson::as_string(data["realm"]));
        lj::Uuid realm(password_hash_method,
                realm_in.c_str(), 
                realm_in.size());

        // Compute the login.
        const std::string login_in(lj::bson::as_string(data["login"]));
        lj::Uuid login(realm,
                login_in.c_str(),
                login_in.size());

        // Compute the pword.
        const std::string pword_in(lj::bson::as_string(data["pword"]));
        lj::Uuid pword(login,
                pword_in.c_str(),
                pword_in.size());

        // Compute the seed.
        if (data.exists("oseed"))
        {
            lj::bson::Binary_type bt;
            uint32_t sz;
            const uint8_t* oseed_in = lj::bson::as_binary(data["oseed"],
                    &bt,
                    &sz);
            return lj::Uuid(pword, oseed_in, sz);
        }
        else
        {
            return lj::Uuid(pword, pword_in.c_str(), pword_in.size());
        }
    }
};
namespace logjamd
{
    Auth_method_password_hash::Auth_method_password_hash()
    {
    }
    Auth_method_password_hash::~Auth_method_password_hash()
    {
    }
    User* Auth_method_password_hash::authenticate(const lj::bson::Node& data)
    {
        lj::Uuid cred(compute_digest(data));
        auto iter = users_by_credential_.find(cred);
        if (iter == users_by_credential_.end())
        {
            lj::Log::debug("User not found for %s.")
                    << static_cast<std::string>(cred)
                    << lj::Log::end;
            // Credentials didn't match anyone.
            return NULL;
        }
        const lj::bson::Node* ptr = (*iter).second;

        const std::string input(lj::bson::as_string(data["login"]));
        const std::string expected(lj::bson::as_string(ptr->nav("login")));
        if (input.compare(expected) != 0)
        {
            lj::Log::debug("Double check failed for %s.")
                    << static_cast<std::string>(cred)
                    << lj::Log::end;
            return NULL;
        }

        const lj::Uuid id(lj::bson::as_uuid(ptr->nav("id")));
        lj::Log::debug("Authenticated user %llu.")
                << static_cast<uint64_t>(id)
                << lj::Log::end;
        return new User(id);
    }

    void Auth_method_password_hash::change_credentials(const User* requester,
            const User* target,
            const lj::bson::Node& data)
    {
        // TODO Something should be done with requester.

        lj::Log::debug.log("auth_local: Finding existing user for %s",
                target->id());
        auto iter = users_by_id_.find(target->id());
        lj::bson::Node* ptr;
        if (users_by_id_.end() == iter)
        {
            lj::Log::debug.log("auth_local: No user found. creating record.");
            ptr = new lj::bson::Node();
            ptr->set_child("id", lj::bson::new_uuid(target->id()));
            ptr->set_child("cred", lj::bson::new_uuid(lj::Uuid::k_nil));
            users_by_id_[target->id()] = ptr;
        }
        else
        {
            ptr = (*iter).second;
        }
        
        lj::Log::debug.log("auth_local: calculating digest for user.");
        lj::bson::Node* new_login = new lj::bson::Node(data["login"]);
        lj::Uuid new_cred(compute_digest(data));

        // check if there is an old record to remove.
        if (users_by_id_.end() != iter)
        {
            lj::Uuid old_cred(lj::bson::as_uuid(ptr->nav("cred")));
            lj::Log::debug("auth_local: Removing old record for %s / %llu")
                    << static_cast<std::string>(old_cred)
                    << static_cast<uint64_t>(target->id())
                    << lj::Log::end;
            users_by_credential_.erase(old_cred);
        }

        // Record the new credential and mapping.
        lj::Log::debug("auth_local: Creating new record for %s / %llu")
                << static_cast<std::string>(new_cred)
                << static_cast<uint64_t>(target->id())
                << lj::Log::end;
        ptr->set_child("cred", lj::bson::new_uuid(new_cred));
        ptr->set_child("login", new_login);
        users_by_credential_[new_cred] = ptr;
    }
}; // namespace logjamd

namespace logjamd
{
    Auth_provider_local::Auth_provider_local()
    {
        lj::Log::info.log("Adding the Local auth provider.");
        Auth_registry::enable(this);
    }
    Auth_provider_local::~Auth_provider_local()
    {
    }
    const lj::Uuid& Auth_provider_local::provider_id() const
    {
        static const lj::Uuid id(k_auth_provider, "local", 5);
        return id;
    }
    Auth_method* Auth_provider_local::method(const lj::Uuid& method_id)
    {
        static Auth_method_password_hash method;
        if (password_hash_method == method_id)
        {
            return &method;
        }
        return NULL;
    }
}; // namespace logjamd

