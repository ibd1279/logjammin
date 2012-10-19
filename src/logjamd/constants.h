#pragma once
/*!
 \file logjamd/constants.h
 \brief Logjam server constants definition.
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

#include "lj/Uuid.h"

namespace logjamd
{
    //! Root Uuid for calculating other Uuids.
    const lj::Uuid k_logjamd_root(lj::Uuid::k_nil, "logjamd", 7);

    //! Root Uuid for calculating the Uuids for authentication methods.
    const lj::Uuid k_auth_method(k_logjamd_root, "auth_method", 11);

    //! Password Hash authentication method Uuid.
    const lj::Uuid k_auth_method_password(logjamd::k_auth_method, "password_hash", 13);

    //! Root Uuid for calculating the Uuids for authentication providers.
    const lj::Uuid k_auth_provider(k_logjamd_root, "auth_provider", 13);

    //! Local authentication provider Uuid.
    const lj::Uuid k_auth_provider_local(k_auth_provider, "local", 5);

    // JSON insecure account.
    const lj::Uuid k_user_id_json("00000000-0000-4006-8fbc-ee299933509f"); //!< built-in json user id.
    const std::string k_user_login_json("99_json_limited"); //!< built-in json user login.
    const std::string k_user_password_json("99_lame_insecure_account"); //!< built-in json user password.

    // HTTP insecure account.
    const lj::Uuid k_user_id_http("00000000-0000-4006-8952-d05d3161ec80"); //!< built-in http user id.
    const std::string k_user_login_http("98_http_limited"); //!< built-in http user login.
    const std::string k_user_password_http("98_lame_insecure_account"); //!< built-in http user password.
};
