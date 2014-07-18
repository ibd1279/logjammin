#pragma once
/*!
 \file logjamd/Environs.h
 \brief Logjam Server environment header.
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

#include "lj/Bson.h"

namespace logjamd
{
    //! Object representing the global server context.
    class Environs {
    public:
        //! Construct a new Server Environment.
        /*!
         \par
         All servers are required to have some form of configuration
         \param config The configuration for the server.
         */
        Environs(lj::bson::Node&& config) :
                config_(config)
        {
        }

        //! Deleted copy constructor.
        Environs(const Environs& orig) = delete;

        //! Deleted copy assignment operator.
        Environs& operator=(const Environs& orig) = delete;

        //! Move constructor.
        Environs(Environs&& orig) :
                config_(std::move(orig.config_)
        {
        }

        //! Move assignment operator.
        Environs& operator=(Environs&& orig)
        {
            config_ = std::move(orig.config_);
        }


        //! Destructor
        virtual ~Environs()
        {
        }

        //! Read only configuration.
        virtual const lj::bson::Node& cfg() const
        {
            return *config_;
        }

    private:
        lj::bson::Node config_;
    }; // class Environs
}; // namespace logjamd
