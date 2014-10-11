/*!
 \file logjam/User.cpp
 \brief Logjam User implementation.
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

#include "logjam/User.h"
#include "lj/Log.h"
#include <sstream>

namespace logjam
{
    const User User::k_unknown(lj::Uuid::k_nil, "Unknown User");

    User::User(const lj::Uuid& user_id,
            const std::string& name) :
            id_(user_id),
            name_(name)
    {
    }

    User::~User()
    {
    }

    const lj::Uuid& User::id() const
    {
        return id_;
    }

    const std::string& User::name() const
    {
        return name_;
    }

    User_not_found_exception::User_not_found_exception(const std::string msg) :
            lj::Exception("User", msg)
    {
    }

    User_not_found_exception::~User_not_found_exception() throw()
    {
    }

    User_repository::~User_repository()
    {
    }

    User User_repository::find(const lj::Uuid& id) const
    {
        auto iter = repository_.find(id);
        if (iter == repository_.end())
        {
            std::ostringstream oss;
            oss << "No user found for " << static_cast<std::string>(id);
            throw User_not_found_exception(oss.str());
        }
        return iter->second;
    }

    void User_repository::store(const User& user)
    {
        repository_.insert(std::map<lj::Uuid, User>::value_type(user.id(), user));
    }
    
    Authentication_method::~Authentication_method()
    {
    }

    Authentication_method_not_found_exception::Authentication_method_not_found_exception(
            const std::string msg) :
            lj::Exception("Authentication_method", msg)
    {
    }

    Authentication_method_not_found_exception::~Authentication_method_not_found_exception() throw()
    {
    }

    Authentication_provider::~Authentication_provider()
    {
    }

    Authentication_provider_not_found_exception::Authentication_provider_not_found_exception(
            const std::string msg) :
            lj::Exception("Authentication_provider", msg)
    {
    }

    Authentication_repository::~Authentication_repository()
    {
        for (auto& p : repository_)
        {
            delete p.second;
        }
    }

    Authentication_provider& Authentication_repository::provider(
            const std::string& provider_name)
    {
        lj::log::Logger& logger =
                lj::log::format<lj::Debug>("Looking up provider [%s] in set: ");
        logger << provider_name << repository_ << lj::log::end;

        auto iter = repository_.find(provider_name);
        if (iter == repository_.end())
        {
            throw Authentication_provider_not_found_exception(provider_name + "is not enabled.");
        }
        return *(iter->second);
    }

    const Authentication_provider& Authentication_repository::provider(const std::string& provider_name) const
    {
        lj::log::Logger& logger =
                lj::log::format<lj::Debug>("Looking up provider [%s] in set: ");
        logger << provider_name << repository_ << lj::log::end;

        auto iter = repository_.find(provider_name);
        if (iter == repository_.end())
        {
            throw Authentication_provider_not_found_exception(provider_name + "is not enabled.");
        }
        return *(iter->second);
    }

    std::unique_ptr<Authentication_provider> Authentication_repository::enable(
            Authentication_provider* ptr)
    {
        lj::log::Logger& logger =
                lj::log::format<lj::Debug>("Adding provider [%s]");
        logger << ptr->name();

        auto iter = repository_.find(ptr->name());
        if (iter == repository_.end())
        {
            repository_.insert(std::map<std::string, Authentication_provider*>::value_type(
                    ptr->name(),
                    ptr));
            return std::unique_ptr<Authentication_provider>();
        }
        std::unique_ptr<Authentication_provider> result(iter->second);
        iter->second = ptr;
        return result;
    }
}; // namespace logjamd

bool operator==(const logjam::User& lhs, const logjam::User& rhs)
{
    return lhs.id() == rhs.id();
}

bool operator!=(const logjam::User& lhs, const logjam::User& rhs)
{
    return lhs.id() != rhs.id();
}
