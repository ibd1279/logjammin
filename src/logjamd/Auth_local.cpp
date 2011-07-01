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

#include "logjamd/Auth.h"
#include "logjamd/constants.h"

namespace logjamd
{
    std::map<lj::Uuid, Auth_provider*> Auth_registry::mapping_;

    class Auth_method_password_hash : public Auth_method
    {
    public:
        virtual ~Auth_method_password_hash()
        {
        }
        virtual User* authenticate(const lj::bson::Node& data)
        {
            return NULL;
        }
    };

    class Auth_provider_local : public Auth_provider
    {
    public:
        Auth_provider_local()
        {
            Auth_registry::enable(this);
        }
        virtual ~Auth_provider_local()
        {
        }
        virtual const lj::Uuid& provider_id()
        {
            static const lj::Uuid id(k_auth_provider, "local", 5);
            return id;
        }
        virtual Auth_method* method(const lj::Uuid& method_id)
        {
            static const lj::Uuid password_hash(k_auth_method, "password_hash", 13);
            static Auth_method_password_hash method;
            if (password_hash == method_id)
            {
                return &method;
            }
            return NULL;
        }
    };
    namespace
    {
        Auth_provider_local _;
    };
};
