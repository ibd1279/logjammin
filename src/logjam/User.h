#pragma once
/*!
 \file logjam/User.h
 \brief Logjam User header.
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
#include "lj/Exception.h"
#include "lj/Uuid.h"
#include <cstdint>

namespace logjam
{
    //! User base class.
    class User
    {
    public:
        static const User k_unknown;
        User(const lj::Uuid& user_id,
                const std::string& name);
        User(const User& orig) = default;
        User(User&& o) = default;
        User& operator=(const User& orig) = default;
        User& operator=(User&& o) = default;

        //! Empty destructor
        virtual ~User();

        //! Get the id for this user.
        virtual const lj::Uuid& id() const;

        //! Get the name for this user.
        virtual const std::string& name() const;
    private:
        lj::Uuid id_;
        std::string name_;
    }; // class logjam::User

    //! User not found exception.
    class User_not_found_exception : public lj::Exception
    {
    public:
        explicit User_not_found_exception(
                const std::string msg);
        User_not_found_exception(
                const User_not_found_exception& o) = default;
        User_not_found_exception(
                User_not_found_exception&& o) = default;
        User_not_found_exception& operator=(
                const User_not_found_exception& rhs) = default;
        User_not_found_exception& operator=(
                User_not_found_exception&& rhs) = default;
        virtual ~User_not_found_exception() throw();
    }; // class logjam::User_not_found_exception

    //! User repository.
    class User_repository
    {
    public:
        User_repository() = default;
        User_repository(const User_repository& o) = delete;
        User_repository(User_repository&& o) = default;
        User_repository& operator=(const User_repository& rhs) = delete;
        User_repository& operator=(User_repository&& rhs) = default;
        virtual ~User_repository();

        virtual User find(const lj::Uuid& id) const;
        virtual void store(const User& user);
    private:
        std::map<lj::Uuid, User> repository_;
    }; // class logjam::User_repository

    //! Abstract authentication method.
    class Authentication_method
    {
    public:
        Authentication_method() = default;
        Authentication_method(const Authentication_method& o) = default;
        Authentication_method(Authentication_method&& o) = default;
        Authentication_method& operator=(const Authentication_method& rhs) = default;
        Authentication_method& operator=(Authentication_method&& rhs) = default;
        virtual ~Authentication_method();

        virtual lj::Uuid authenticate(const lj::bson::Node& data) const = 0;
        virtual void change_credential(const lj::Uuid& target,
                const lj::bson::Node& data) = 0;
        virtual std::string name() const = 0;
    }; // class logjam::Authentication_method

    //! Authentication method not found exception.
    class Authentication_method_not_found_exception : public lj::Exception
    {
    public:
        explicit Authentication_method_not_found_exception(
                const std::string msg);
        Authentication_method_not_found_exception(
                const Authentication_method_not_found_exception& o) = default;
        Authentication_method_not_found_exception(
                Authentication_method_not_found_exception&& o) = default;
        Authentication_method_not_found_exception& operator=(
                const Authentication_method_not_found_exception& rhs) = default;
        Authentication_method_not_found_exception& operator=(
                Authentication_method_not_found_exception&& rhs) = default;
        virtual ~Authentication_method_not_found_exception() throw();
    }; // class logjam::Authentication_method_not_found_exception

    //! Abstract authentication provider.
    class Authentication_provider
    {
    public:
        Authentication_provider() = default;
        Authentication_provider(const Authentication_provider& o) = default;
        Authentication_provider(Authentication_provider&& o) = default;
        Authentication_provider& operator=(const Authentication_provider& rhs) = default;
        Authentication_provider& operator=(Authentication_provider&& rhs) = default;
        virtual ~Authentication_provider();

        virtual Authentication_method& method(const std::string& method_name) = 0;
        virtual const Authentication_method& method(const std::string& method_name) const = 0;
        virtual std::string name() const = 0;
    }; // class logjam::Authentication_provider

    //! Simple, single method authentication provider.
    template <class Method>
    class Authentication_provider_simple : public Authentication_provider
    {
    public:
        explicit Authentication_provider_simple(const std::string& name) :
                Authentication_provider(),
                name_(name),
                method_()
        {
        }

        Authentication_provider_simple(
                const Authentication_provider_simple& o) = default;
        Authentication_provider_simple(
                Authentication_provider_simple&& o) = default;
        Authentication_provider_simple& operator=(
                const Authentication_provider_simple& rhs) = default;
        Authentication_provider_simple& operator=(
                Authentication_provider_simple&& rhs) = default;
        virtual ~Authentication_provider_simple() = default;

        virtual Method& method(const std::string& method_name) override
        {
            if (method_name.compare(method_.name()) != 0)
            {
                throw Authentication_method_not_found_exception(method_name);
            }
            return method_;
        }

        virtual const Method& method(const std::string& method_name) const override
        {
            if (method_name.compare(method_.name()) != 0)
            {
                throw Authentication_method_not_found_exception(method_name);
            }
            return method_;
        }

        virtual std::string name() const override
        {
            return name_;
        }
    private:
        std::string name_;
        Method method_;
    }; // class logjam::Authentication_provider_simple

    //! Authentication provider not found exception.
    class Authentication_provider_not_found_exception : public lj::Exception
    {
    public:
        explicit Authentication_provider_not_found_exception(
                const std::string msg);
        Authentication_provider_not_found_exception(
                const Authentication_provider_not_found_exception& o) = default;
        Authentication_provider_not_found_exception(
                Authentication_provider_not_found_exception&& o) = default;
        Authentication_provider_not_found_exception& operator=(
                const Authentication_provider_not_found_exception& rhs) = default;
        Authentication_provider_not_found_exception& operator=(
                Authentication_provider_not_found_exception&& rhs) = default;
        virtual ~Authentication_provider_not_found_exception() throw() = default;
    }; // class logjam::Authentication_provider_not_found_exception

    //! Authentication repository.
    class Authentication_repository
    {
    public:
        Authentication_repository() = default;
        Authentication_repository(const Authentication_repository& o) = delete;
        Authentication_repository(Authentication_repository&& o) = default;
        Authentication_repository& operator=(const Authentication_repository& rhs) = delete;
        Authentication_repository& operator=(Authentication_repository&& rhs) = default;
        virtual ~Authentication_repository();

        virtual Authentication_provider& provider(const std::string& provider_name);
        virtual const Authentication_provider& provider(const std::string& provider_name) const;
        virtual std::unique_ptr<Authentication_provider> enable(Authentication_provider* ptr);
    private:
        std::map<std::string, Authentication_provider*> repository_;
    }; // class logjam::Authentication_repository
}; // namespace logjam

bool operator==(const logjam::User& lhs, const logjam::User& rhs);
bool operator!=(const logjam::User& lhs, const logjam::User& rhs);
