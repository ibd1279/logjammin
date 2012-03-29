#pragma once
/*!
 \file logjamd/Auth.h
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

#include "lj/Bson.h"
#include "lj/Uuid.h"
#include "lj/Log.h"
#include "logjamd/User.h"
#include <cstdint>
#include <map>

namespace logjamd
{
    //! Authentication method base class.
    class Auth_method
    {
    public:
        virtual ~Auth_method() {}
        
        //! Perform authentication.
        /*!
         \par
         All specific login functionality required for the authentication
         method must be implmented in this method.
         \par
         User is expected to be allocated with \c new.
         \param data The user provided login data.
         \return NULL on login failure, A User pointer on success.
         */
        virtual User* authenticate(const lj::bson::Node& data) = 0;

        //! Change the login credentials for the specific target user.
        /*!
         \par
         This method changes the credentials for a target user. This will
         modify the record for the user immediately and will impact all
         current connections for that user.
         \param requester The user requesting the change.
         \param target The user to change the credentials for.
         \param data The new credential data.
         */
        virtual void change_credentials(const User* requester,
                const User* target,
                const lj::bson::Node& data) = 0;
    };

    //! Authentication provider base class.
    class Auth_provider
    {
    public:
        //! Empty destructor
        virtual ~Auth_provider() {}

        //! Return the id of this provider.
        /*!
         \par
         The provider id should be a version 5 Uuid created from the
         logjamd::k_auth_provider constant and the name of the provider.
         \return the ID for this provider.
         */
        virtual const lj::Uuid& provider_id() const = 0;
        
        //! Check to see if the provider supports the requested method.
        /*!
         \par
         Some providers (like the Auth_provider_local) support multiple
         different authentication methods. This is used to select the
         method requested by the end user.
         \par
         If a provider does not support multiple methods, they are
         suggested to use lj::Uuid::k_nil for the method_id they accept.
         \par
         The method_id should be a version 5 Uuid created from the
         logjamd::k_auth_method constant and the name of the method.
         \param method_id The requested method_id.
         \return NULL if unsupported methods, an Auth_method pointer on
         success.
         */
        virtual Auth_method* method(const lj::Uuid& method_id) = 0;
    };

    //! Registry of authentication providers.
    /*!
     \par
     Namespace for methods to register and look up authentication
     providers.
     \par
     Once the provider registry has been locked, new authentication providers
     cannot be added to the registry.
     */
    class Auth_registry
    {
    public:
        //! Enable an auth provider.
        /*!
         \par
         This method can only add authentication providers to the registry
         while the registry is not locked.
         \param p The provider to add to the registry.
         \return true on success, false on failure.
         */
        static bool enable(Auth_provider* p)
        {
            mapping_[p->provider_id()] = p;
            return true;
        }

        //! Look up an authentication provider.
        /*!
         \param id The provider id.
         \return NULL if unknown provider, an Auth_provider pointer on success.
         */
        static Auth_provider* const provider(const lj::Uuid& id)
        {
            auto iter = mapping_.find(id);
            if (mapping_.end() == iter)
            {
                lj::log::format<lj::Info>("Provider %s not found.").end(
                        static_cast<std::string>(id));
                return nullptr;
            }

            return (*iter).second;
        }
    private:
        static std::map<lj::Uuid, Auth_provider*> mapping_;
    };
};
