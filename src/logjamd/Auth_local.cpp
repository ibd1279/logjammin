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

#include "logjamd/Auth_local.h"
//#include "logjamd/constants.h"
#include "lj/Log.h"
#include "scrypt/scrypt.h"

#include <cstdlib>
#include <cstring>

namespace
{
    const std::string k_login_field("login");
    const std::string k_password_field("password");
    const std::string k_id_field("id");
    const std::string k_salt_field("salt");

    const std::string k_password_hash_name("bcrypt");

    // XXX These values should come from the build configure.
    const size_t k_derived_key_length = 128;
    const int k_N = 1 << 12;
    const int k_r = 8;
    const int k_p = 1;
};

namespace logjamd
{
    Auth_method_password_hash::Auth_method_password_hash()
    {
    }

    Auth_method_password_hash::~Auth_method_password_hash()
    {
        for (auto iter = credentials_by_id_.begin();
                credentials_by_id_.end() != iter;
                ++iter)
        {
            delete (*iter).second;
        }
    }

    lj::Uuid Auth_method_password_hash::authenticate(const lj::bson::Node& data) const
    {
        const std::string login(lj::bson::as_string(data[k_login_field]));
        auto iter = credentials_by_login_.find(login);
        if (iter == credentials_by_login_.end())
        {
            // Deal with unknown login.
            lj::log::format<lj::Debug>("auth_local: User not found for %s.")
                    << login
                    << lj::log::end;
            throw logjam::User_not_found_exception(login);
        }
        const lj::bson::Node* stored_credential = (*iter).second;

        // Prepare scrypt inputs
        lj::log::format<lj::Debug>("auth_local: Calculating derived key.");
        const size_t password_length = data[k_password_field].size();
        const uint8_t* password = data[k_password_field].to_value();
        const size_t salt_length = stored_credential->nav(k_salt_field).size();
        const uint8_t* salt = stored_credential->nav(k_salt_field).to_value();

        // calculate the derived key.
        uint8_t derived_key[k_derived_key_length];
        crypto_scrypt(password,
                password_length,
                salt,
                salt_length,
                k_N,
                k_r,
                k_p,
                derived_key,
                k_derived_key_length);

        // Get the stored derived key.
        lj::bson::Binary_type bin_type = lj::bson::Binary_type::k_bin_generic;
        uint32_t check_key_length = 0;
        const uint8_t* check_key = lj::bson::as_binary(stored_credential->nav(k_password_field),
                &bin_type,
                &check_key_length);

        // Compare the two keys. Abort if they don't match.
        for (size_t h = 0; h < k_derived_key_length; ++h)
        {
            if (derived_key[h] != check_key[h])
            {
                lj::log::format<lj::Debug>(
                        "auth_local: Credentials did not match for %s.")
                        << login
                        << lj::log::end;
                throw logjam::User_not_found_exception(login);
            }
        }

        // Login successful, return the user object.
        lj::log::format<lj::Debug>("auth_local: Authenticated user %s.")
                << login
                << lj::log::end;
        return lj::bson::as_uuid(stored_credential->nav(k_id_field));
    }

    void Auth_method_password_hash::change_credential(const lj::Uuid& target,
            const lj::bson::Node& data)
    {
        lj::log::format<lj::Debug>("auth_local: Finding existing user for %s")
                << target
                << lj::log::end;
        auto iter = credentials_by_id_.find(target);

        lj::bson::Node* stored_credential;
        if (credentials_by_id_.end() == iter)
        {
            lj::log::out<lj::Debug>(
                    "auth_local: No user found. creating record.");
            stored_credential = new lj::bson::Node();
            uint8_t temp_dk[1] = {0};
            stored_credential->set_child(k_id_field,
                    lj::bson::new_uuid(target));
            stored_credential->set_child(k_login_field,
                    lj::bson::new_string(""));
            stored_credential->set_child(k_password_field,
                    lj::bson::new_binary(temp_dk, 1, lj::bson::Binary_type::k_bin_generic));
            stored_credential->set_child(k_salt_field,
                    lj::bson::new_binary(temp_dk, 1, lj::bson::Binary_type::k_bin_generic));
            credentials_by_id_[target] = stored_credential;
        }
        else
        {
            stored_credential = (*iter).second;
        }

        lj::log::out<lj::Debug>("auth_local: calculating new derived key.");
        // read a new random salt.
        // TODO this is insecure. should use something much more secure
        // for creating salts.
        uint8_t salt_buffer[128];
        for (int h = 0; h < 128; ++h)
        {
            salt_buffer[h] = (rand() & 0xFF);
        }
        lj::bson::Node* salt_node = lj::bson::new_binary(
                salt_buffer,
                128,
                lj::bson::Binary_type::k_bin_generic);

        // The salt and password include the bson header info. This is
        // intentional, because it reduces the code complexity.
        const size_t salt_length = salt_node->size();
        const uint8_t* salt = salt_node->to_value();
        const size_t password_length = data[k_password_field].size();
        const uint8_t* password = data[k_password_field].to_value();

        // calculate the derived key.
        uint8_t derived_key[k_derived_key_length];
        crypto_scrypt(password,
                password_length,
                salt,
                salt_length,
                k_N,
                k_r,
                k_p,
                derived_key,
                k_derived_key_length);

        // check if there is an old record to remove.
        if (credentials_by_id_.end() != iter)
        {
            const std::string old_login(
                    lj::bson::as_string(stored_credential->nav(k_login_field)));
            lj::log::format<lj::Debug>("auth_local: Removing old record for %s / %s")
                    << old_login
                    << target
                    << lj::log::end;
            credentials_by_login_.erase(old_login);
        }

        // In theory, the user cannot log in during this point. There is no entry in the
        // users_by_credential_ to look up this user.

        // TODO Existing sessions should probably be disconnected here.

        // Record the new credential and mapping.
        const std::string new_login(lj::bson::as_string(data[k_login_field]));
        lj::log::format<lj::Debug>("auth_local: Creating new record for %s / %s")
                << new_login
                << target
                << lj::log::end;

        stored_credential->set_child(k_login_field,
                lj::bson::new_string(new_login));
        stored_credential->set_child(k_password_field,
                lj::bson::new_binary(derived_key,
                        k_derived_key_length,
                        lj::bson::Binary_type::k_bin_generic));
        stored_credential->set_child(k_salt_field,
                salt_node);

        credentials_by_login_[new_login] = stored_credential;
    }

    std::string Auth_method_password_hash::name() const
    {
        return k_password_hash_name;
    }
}; // namespace logjamd
