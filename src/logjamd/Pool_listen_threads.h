#pragma once
/*!
 \file logjamd/Pool_listen_threads.h
 \brief Logjam server listening threaded pool header.
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

#include "logjam/Network_connection.h"
#include "logjam/Pool.h"
#include <atomic>
#include <map>

namespace logjamd
{
    namespace pool
    {
        //! Thread-per connection swimmer. Socket listener
        class Swimmer_listener : public logjam::pool::Swimmer
        {
        public:
            Swimmer_listener(logjam::pool::Lifeguard& lg,
                    logjam::Context&& ctx,
                    int sockfd);
            Swimmer_listener(const Swimmer_listener& o) = delete;
            Swimmer_listener(Swimmer_listener&& o) = delete;
            Swimmer_listener& operator=(const Swimmer_listener&& rhs) = delete;
            Swimmer_listener& operator=(Swimmer_listener&& rhs) = delete;
            virtual ~Swimmer_listener() = default;

            virtual void run() override;
            virtual void stop() override;
            virtual void cleanup() override;
            virtual std::iostream& io() override;
        private:
            std::atomic<bool> is_running_;
            logjam::Network_connection client_connection_;
        }; // class logjamd::pool::Swimmer_listener

        //! Thread-per connection lifeguard. Socket listener
        class Lifeguard_listener : public logjam::pool::Lifeguard
        {
        public:
            Lifeguard_listener(logjam::pool::Area& a,
                    int sockfd);
            Lifeguard_listener(const Lifeguard_listener& o) = delete;
            Lifeguard_listener(Lifeguard_listener&& o) = delete;
            Lifeguard_listener& operator=(const Lifeguard_listener& rhs) = delete;
            Lifeguard_listener& operator=(Lifeguard_listener&& rhs) = delete;
            virtual ~Lifeguard_listener();

            virtual void remove(logjam::pool::Swimmer* s) override;
            virtual void watch(logjam::pool::Swimmer* s) override;
            virtual void run() override;
            virtual void stop();
            virtual void cleanup() override;
        private:
            std::atomic<bool> is_running_;
            typedef std::map<logjam::pool::Swimmer*, lj::Thread> Swimmer_map;
            Swimmer_map responsibilities_;
            logjam::Network_connection listen_connection_;
        }; // class logjamd::pool::Lifeguard_listener

        //! Thread-per connection area. Socket Listener.
        class Area_listener : public logjam::pool::Area
        {
        public:
            explicit Area_listener(logjam::Environs&& env);
            Area_listener(const Area_listener& o) = delete;
            Area_listener(Area_listener&& o) = default;
            Area_listener& operator=(const Area_listener& rhs) = delete;
            Area_listener& operator=(Area_listener&& rhs) = default;
            virtual ~Area_listener() = default;

            virtual void prepare() override;
            virtual void open() override;
            virtual void close() override;
            virtual void cleanup() override;
        private:
            lj::Thread lifeguard_thread_;
            std::unique_ptr<Lifeguard_listener> lifeguard_;
        }; // class logjamd::pool::Area_listener
    }; // namespace logjamd::pool
}; // namespace logjamd
