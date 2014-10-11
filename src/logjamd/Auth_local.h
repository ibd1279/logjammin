#pragma once
/*!
 \file logjamd/Auth_local.h
 \brief Logjam server authentication header.
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

#include "logjam/User.h"

namespace logjamd
{
    //! Local Authentication password hash method implementation.
    class Auth_method_password_hash : public logjam::Authentication_method
    {
    public:
        Auth_method_password_hash();
        Auth_method_password_hash(const Auth_method_password_hash& o) = delete;
        Auth_method_password_hash(Auth_method_password_hash&& o) = default;
        Auth_method_password_hash& operator=(
                const Auth_method_password_hash& rhs) = delete;
        Auth_method_password_hash& operator=(
                Auth_method_password_hash&& rhs) = default;
        virtual ~Auth_method_password_hash();
        virtual lj::Uuid authenticate(const lj::bson::Node& data) const override;
        virtual void change_credential(const lj::Uuid& target,
                const lj::bson::Node& data);
        virtual std::string name() const;
    private:
        std::map<std::string, lj::bson::Node*> credentials_by_login_;
        std::map<lj::Uuid, lj::bson::Node*> credentials_by_id_;
    };
};
