/*!
 \file logjamd/Pool_listen_threads.cpp
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

#include "logjamd/Pool_listen_threads.h"
#include "logjamd/Server.h"
#include "logjamd/Stage_pre.h"
#include "logjam/Network_address_info.h"
#include <memory>

namespace logjamd
{
    namespace pool
    {
        //// Swimmer_listener

        Swimmer_listener::Swimmer_listener(logjam::pool::Lifeguard& lg,
                logjam::Context&& ctx,
                int sockfd) :
                logjam::pool::Swimmer(lg, std::move(ctx)),
                is_running_(false),
                client_connection_(sockfd)
        {
        }

        void Swimmer_listener::run()
        {
            is_running_.store(true);
            std::unique_ptr<logjam::Stage> stage(
                    new logjamd::Stage_pre());

            while (is_running_.load() && nullptr != stage)
            {
                try
                {
                    stage = safe_execute_stage(stage, *this);
                    io().flush();
                }
                catch (const lj::Exception& ex)
                {
                    stage.reset();
                    lj::log::format<lj::Error>("Encountered %s LJ Exception.")
                            << ex
                            << lj::log::end;
                }
                catch (const std::exception& ex)
                {
                    stage.reset();
                    lj::log::format<lj::Critical>("Encountered %s std Exception.")
                            << ex
                            << lj::log::end;
                }
                catch (...)
                {
                    stage.reset();
                    lj::log::out<lj::Alert>("Encountered an unexpected Exception.");
                }
            }

            is_running_.store(false);
            lj::log::out<lj::Debug>("Swimmer Thread Exited.");
        }

        void Swimmer_listener::stop()
        {
            is_running_.store(false);
        }

        void Swimmer_listener::cleanup()
        {
            lj::log::format<lj::Debug>("Swimmer %p cleaned up.")
                    << this
                    << lj::log::end;
            lifeguard().remove(this);
            delete this;
        }

        std::iostream& Swimmer_listener::io()
        {
            return client_connection_.stream();
        }

        //// Lifeguard_listener

        Lifeguard_listener::Lifeguard_listener(logjam::pool::Area& a,
                int sockfd) :
                logjam::pool::Lifeguard(a),
                is_running_(false),
                responsibilities_(),
                listen_connection_(sockfd)
        {
        }

        Lifeguard_listener::~Lifeguard_listener()
        {
            for (Swimmer_map::value_type& p : responsibilities_)
            {
                // call stop, wait for it to finish, then delete.
                p.first->stop();
                p.second.join();
                delete p.first;
            }
        }

        void Lifeguard_listener::remove(logjam::pool::Swimmer* s)
        {
            Swimmer_map::iterator iter(responsibilities_.find(s));
            if (responsibilities_.end() != iter)
            {
                iter->first->stop();
                responsibilities_.erase(iter);
            }
        }

        void Lifeguard_listener::watch(logjam::pool::Swimmer* s)
        {
            Swimmer_map::iterator iter(responsibilities_.find(s));
            if (responsibilities_.end() == iter)
            {
                responsibilities_[s].run(s);
            }
        }

        void Lifeguard_listener::run()
        {
            is_running_.store(true);
            while(is_running_.load())
            {
                // Accept a connection.
                struct sockaddr_storage remote_addr;
                socklen_t remote_addr_size = sizeof(struct sockaddr_storage);
                int sockfd = accept(listen_connection_.socket(),
                        (struct sockaddr *)&remote_addr,
                        &remote_addr_size);
                if (0 > sockfd)
                {
                    // I had problems accepting that client.
                    throw LJ__Exception(strerror(errno));
                }

                // Create the swimmer.
                Swimmer_listener* new_swimmer = new Swimmer_listener(*this,
                        area().spawn_context(),
                        sockfd);

                // Collect all the admin stuff we need for this connection.
                std::string remote_ip = logjam::Network_address_info::as_string(
                        (struct sockaddr*)&remote_addr);
                new_swimmer->context().node().set_child("client/address",
                        lj::bson::new_string(remote_ip));

                // Start watching the swimmer.
                watch(new_swimmer);

                lj::log::format<lj::Info>("Accepted a connection from %s on fh %d.")
                        << remote_ip
                        << sockfd
                        << lj::log::end;
            }
        }

        void Lifeguard_listener::stop()
        {
            is_running_.store(false);
        }

        void Lifeguard_listener::cleanup()
        {
        }

        //// Area_listener

        Area_listener::Area_listener(logjam::Environs&& env) :
                logjam::pool::Area(std::move(env)),
                lifeguard_thread_(),
                lifeguard_(nullptr)
        {
        }

        void Area_listener::prepare()
        {
            // Figure out where we should be listening.
            std::string listen_on(lj::bson::as_string(
                    environs().config()["server/listen"]));
            lj::log::format<lj::Info>("Attempting to listen on \"%s\".")
                    << listen_on
                    << lj::log::end;

            logjam::Network_address_info info(listen_on,
                    AI_PASSIVE,
                    AF_UNSPEC,
                    SOCK_STREAM,
                    0);
            if (!info.next())
            {
                // we didn't get any address information back, so abort!
                throw LJ__Exception(info.error());
            }

            // Now create my socket descriptor for listening.
            int sockfd = ::socket(info.current().ai_family,
                    info.current().ai_socktype,
                    info.current().ai_protocol);
            if (0 > sockfd)
            {
                // Did not get a socket descriptor.
                throw LJ__Exception(strerror(errno));
            }

            int rc = ::bind(sockfd,
                    info.current().ai_addr,
                    info.current().ai_addrlen);
            if (0 > rc)
            {
                // Did not get a socket descriptor.
                throw LJ__Exception(strerror(errno));
            }

            rc = ::listen(sockfd, 5);
            if (0 > rc)
            {
                // did not bind the listener to a port.
                throw LJ__Exception(strerror(errno));
            }

            lifeguard_.reset(new Lifeguard_listener(*this, sockfd));
        }

        void Area_listener::open()
        {
            assert(nullptr != lifeguard_);
            //lifeguard_thread_.run(&lifeguard_);
            lifeguard_->run();
        }

        void Area_listener::close()
        {
            assert(nullptr != lifeguard_);
            lifeguard_->stop();
        }

        void Area_listener::cleanup()
        {
            assert(nullptr != lifeguard_);
            lifeguard_thread_.join();
        }

    }; // namespace logjamd::pool
}; // namespace logjamd
