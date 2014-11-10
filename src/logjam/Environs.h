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
    /*!
     \brief Object representing the global pool context.
     \since 1.0
     */
    class Environs {
    public:
        /*!
         \brief Construct a new Pool Environment.

         All Pools are required to have some form of configuration

         \param config The configuration for the server.
         */
        Environs(lj::bson::Node&& cfg,
                User_repository* ur,
                Authentication_repository* ar);

        //! Deleted copy constructor.
        Environs(const Environs& orig) = delete;

        //! Default move constructor.
        Environs(Environs&& orig) = default;

        //! Deleted copy assignment operator.
        Environs& operator=(const Environs& orig) = delete;

        //! Default move assignment operator.
        Environs& operator=(Environs&& orig) = default;

        //! Destructor.
        virtual ~Environs() = default;

        //! Get the reference to the configuration.
        virtual const lj::bson::Node& config() const;

        //! Get the reference to the user repository.
        virtual User_repository& user_repository();

        //! Get the reference to the authentication repository.
        virtual Authentication_repository& authentication_repository();

    private:
        lj::bson::Node config_;
        User_repository* user_repository_;
        Authentication_repository* authentication_repository_;
    }; // class lj::Environs

    /*!
     \brief Object representing the swimmer context.
     \since 1.0
     */
    class Context
    {
    public:
        /*!
         \brief Base class for additioanl data stored in the Context.
         \since 1.0
         */
        struct Additional_data
        {
            //! Default constructor.
            Additional_data() = default;

            //! Default copy constructor.
            Additional_data(const Additional_data& o) = default;

            //! Default move constructor.
            Additional_data(Additional_data&& o) = default;

            //! Default copy assignment operator.
            Additional_data& operator=(const Additional_data& rhs) = default;

            //! Default move assignment operator.
            Additional_data& operator=(Additional_data&& rhs) = default;

            //! Destructor.
            virtual ~Additional_data() = default;
        }; // struct logjam::Context::Additional_data;

        /*!
         \brief Create a new context object.
         \param environs The parent environment.
         */
        Context(std::shared_ptr<Environs>& environs);
        
        //! Default copy constructor.
        Context(const Context& o) = default;

        //! Default move constructor.
        Context(Context&& o) = default;

        //! Default copy assignment operator.
        Context& operator=(const Context& rhs) = default;

        //! Default move assignment operator.
        Context& operator=(Context&& rhs) = default;

        //! Destructor.
        virtual ~Context() = default;

        /*!
         \brief Set additional data for the context.

         The context assumes responsibility for releasing the pointer.

         \param ptr Pointer to the additional data.
         */
        virtual void data(Additional_data* ptr);

        //! Get the additional data.
        virtual Additional_data* data();

        //! Get the additional data.
        virtual const Additional_data* data() const;

        //! Get the context bson node.
        virtual lj::bson::Node& node();

        //! Get the context bson node.
        virtual const lj::bson::Node& node() const;

        //! Get the context user.
        virtual logjam::User& user();

        //! Get the context user.
        virtual const logjam::User& user() const;

        //! Get the parent environs.
        virtual logjam::Environs& environs();

        //! Get the parent environs.
        virtual const logjam::Environs& environs() const;
    private:
        std::shared_ptr<Additional_data> data_;
        lj::bson::Node node_;
        logjam::User user_;
        std::shared_ptr<logjam::Environs> environs_;
    }; // class logjam::Context
}; // namespace logjam
