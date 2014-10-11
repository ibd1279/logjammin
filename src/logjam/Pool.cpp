/*!
 \file logjam/Pool.cpp
 \brief Logjam abstract connection pool implementation.
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

#include "logjam/Pool.h"

namespace logjam
{
    namespace pool
    {
        //// Area

        Area::Area(Environs&& env) :
                environs_(new Environs(std::move(env)))
        {
        }

        logjam::Environs& Area::environs() 
        {
            return *environs_;
        }

        const logjam::Environs& Area::environs() const
        {
            return *environs_;
        }

        logjam::Context Area::spawn_context()
        {
            return logjam::Context(environs_);
        }

        //// Lifeguard

        Lifeguard::Lifeguard(Area& a) :
                area_(a)
        {
        }

        Area& Lifeguard::area()
        {
            return area_;
        }

        const Area& Lifeguard::area() const
        {
            return area_;
        }

        //// Swimmer

        Swimmer::Swimmer(Lifeguard& lg, Context&& ctx) :
                lifeguard_(lg),
                context_(std::move(ctx))
        {
        }

        Lifeguard& Swimmer::lifeguard()
        {
            return lifeguard_;
        }

        const Lifeguard& Swimmer::lifeguard() const
        {
            return lifeguard_;
        }

        Context& Swimmer::context()
        {
            return context_;
        }

        const Context& Swimmer::context() const
        {
            return context_;
        }

        namespace utility
        {
            //// Swimmer xlator

            Swimmer_xlator::Swimmer_xlator(Swimmer& parent,
                    std::iostream& io) :
                    logjam::pool::Swimmer(parent.lifeguard(),
                            logjam::Context(parent.context())),
                    parent_(parent),
                    io_(io)
            {
            }

            Swimmer_xlator::~Swimmer_xlator()
            {
            }

            void Swimmer_xlator::run()
            {
                parent().run();
            }

            void Swimmer_xlator::stop()
            {
                parent().stop();
            }

            void Swimmer_xlator::cleanup()
            {
                parent().cleanup();
            }

            std::iostream& Swimmer_xlator::io()
            {
                return io_;
            }

            logjam::Context& Swimmer_xlator::context()
            {
                return parent().context();
            }

            const logjam::Context& Swimmer_xlator::context() const
            {
                return parent().context();
            }

            Swimmer& Swimmer_xlator::parent()
            {
                return parent_;
            }

            const Swimmer& Swimmer_xlator::parent() const
            {
                return parent_;
            }
        }; // namespace logjam::pool::utility
    }; // namespace logjam::pool
}; // namespace logjam
