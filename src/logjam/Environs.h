#pragma once
/*!
 \file logjam/Environs.h
 \brief Logjam Pool environment header.
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
#include "lj/Bson.h"

namespace logjam
{
    //! Object representing the global pool context.
    class Environs {
    public:
        //! Construct a new Pool Environment.
        /*!
         \par
         All Pools are required to have some form of configuration
         \param config The configuration for the server.
         */
        Environs(lj::bson::Node&& cfg,
                User_repository* ur,
                Authentication_repository* ar);
        Environs(const Environs& orig) = delete;
        Environs(Environs&& orig) = default;
        Environs& operator=(const Environs& orig) = delete;
        Environs& operator=(Environs&& orig) = default;
        virtual ~Environs() = default;

        virtual const lj::bson::Node& config() const;
        virtual User_repository& user_repository();
        virtual Authentication_repository& authentication_repository();

    private:
        lj::bson::Node config_;
        User_repository* user_repository_;
        Authentication_repository* authentication_repository_;
    }; // class Environs

    //! Object representing the swimmer context.
    class Context
    {
    public:
        //! Base class for additioanl data stored in the Context.
        struct Additional_data
        {
            Additional_data() = default;
            virtual ~Additional_data() = default;
        }; // struct logjam::Context::Additional_data;

        Context(std::shared_ptr<Environs>& environs);
        Context(const Context& o) = default;
        Context(Context&& o) = default;
        Context& operator=(const Context& rhs) = default;
        Context& operator=(Context&& rhs) = default;
        virtual ~Context() = default;

        virtual void data(Additional_data* ptr);
        virtual Additional_data* data();
        virtual const Additional_data* data() const;
        virtual lj::bson::Node& node();
        virtual const lj::bson::Node& node() const;
        virtual logjam::User& user();
        virtual const logjam::User& user() const;
        virtual logjam::Environs& environs();
        virtual const logjam::Environs& environs() const;
    private:
        std::shared_ptr<Additional_data> data_;
        lj::bson::Node node_;
        logjam::User user_;
        std::shared_ptr<logjam::Environs> environs_;
    }; // class logjam::Context
}; // namespace logjam
