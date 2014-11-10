#pragma once
/*!
 \file logjam/Pool.h
 \brief Logjam abstract connection pool header.
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
#include "lj/Thread.h"

namespace logjam
{
    namespace pool
    {
        class Lifeguard;
        class Swimmer;

        //! Area of the pool.
        class Area
        {
        public:
            explicit Area(Environs&& env);
            Area(const Area& o) = default;
            Area(Area&& o) = default;
            Area& operator=(const Area& rhs) = default;
            Area& operator=(Area&& rhs) = default;
            virtual ~Area() = default;

            virtual void prepare() = 0;
            virtual void open() = 0;
            virtual void close() = 0;
            virtual void cleanup() = 0;

            virtual logjam::Environs& environs();
            virtual const logjam::Environs& environs() const;
            virtual logjam::Context spawn_context();
        private:
            std::shared_ptr<logjam::Environs> environs_;
        }; // class logjam::pool::Area

        //! Lifeguard assigned to areas of the pool.
        class Lifeguard : public lj::Work
        {
        public:
            explicit Lifeguard(Area& a);
            Lifeguard(const Lifeguard& o) = default;
            Lifeguard(Lifeguard&& o) = default;
            Lifeguard& operator=(const Lifeguard& rhs) = default;
            Lifeguard& operator=(Lifeguard&& rhs) = default;
            virtual ~Lifeguard() = default;

            virtual void remove(Swimmer* s) = 0;
            virtual void watch(Swimmer* s) = 0;

            virtual Area& area();
            virtual const Area& area() const;
        private:
            Area& area_;
        }; // class logjam::pool::Lifeguard

        //! Swimmers watched by the lifeguard of the pool.
        class Swimmer : public:: lj::Work
        {
        public:
            Swimmer(Lifeguard& lg, Context&& ctx);
            Swimmer(const Swimmer& o) = delete;
            Swimmer(Swimmer&& o) = default;
            Swimmer& operator=(const Swimmer&& rhs) = delete;
            Swimmer& operator=(Swimmer&& rhs) = default;
            virtual ~Swimmer() = default;

            virtual void stop() = 0;
            virtual std::iostream& io() = 0;

            virtual Lifeguard& lifeguard();
            virtual const Lifeguard& lifeguard() const;
            virtual Context& context();
            virtual const Context& context() const;
        private:
            Lifeguard& lifeguard_;
            Context context_;
        }; // class logjam::pool::Swimmer
        namespace utility
        {
            //! Translate one swimmer into another swimmer.
            class Swimmer_xlator : public Swimmer
            {
            public:
                Swimmer_xlator(Swimmer& parent, std::iostream& io);
                Swimmer_xlator(const Swimmer_xlator& o) = delete;
                Swimmer_xlator(Swimmer_xlator&& o) = default;
                Swimmer_xlator& operator=(const Swimmer_xlator&& rhs) = delete;
                Swimmer_xlator& operator=(Swimmer_xlator&& rhs) = delete;
                virtual ~Swimmer_xlator();

                virtual void run() override;
                virtual void stop() override;
                virtual void cleanup() override;

                virtual std::iostream& io() override;
                virtual Context& context() override;
                virtual const Context& context() const override;

                virtual Swimmer& parent();
                virtual const Swimmer& parent() const;
            private:
                Swimmer& parent_;
                std::iostream& io_;
            }; // class logjam::pool::utility::Swimmer_xlator
        }; // namespace logjam::pool::utility
    }; // namespace logjam::pool
}; // namespace logjam
