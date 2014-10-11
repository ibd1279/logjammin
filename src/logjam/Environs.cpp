/*!
 \file logjam/Environs.cpp
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

#include "logjam/Environs.h"

namespace logjam
{
    Environs::Environs(lj::bson::Node&& cfg,
            User_repository* ur,
            Authentication_repository* ar) :
            config_(std::move(cfg)),
            user_repository_(ur),
            authentication_repository_(ar)
    {
    }

    const lj::bson::Node& Environs::config() const
    {
        return config_;
    }

    User_repository& Environs::user_repository()
    {
        return *user_repository_;
    }

    Authentication_repository& Environs::authentication_repository()
    {
        return *authentication_repository_;
    }

    Context::Context(std::shared_ptr<Environs>& environs) :
            data_(),
            node_(),
            user_(User::k_unknown),
            environs_(environs)
    {
    }

    void Context::data(Context::Additional_data* ptr)
    {
        data_.reset(ptr);
    }

    Context::Additional_data* Context::data()
    {
        return data_.get();
    }

    const Context::Additional_data* Context::data() const
    {
        return data_.get();
    }

    lj::bson::Node& Context::node()
    {
        return node_;
    }

    const lj::bson::Node& Context::node() const
    {
        return node_;
    }

    logjam::User& Context::user()
    {
        return user_;
    }

    const logjam::User& Context::user() const
    {
        return user_;
    }

    logjam::Environs& Context::environs()
    {
        return *environs_;
    }

    const logjam::Environs& Context::environs() const
    {
        return *environs_;
    }

}; // namespace logjam
