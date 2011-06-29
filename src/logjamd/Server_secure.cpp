/*!
 \file logjamd/Server_secure.cpp
 \brief Logjam server networking implementation.
 \author Jason Watson
 Copyright (c) 2011, Jason Watson
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

#include "logjamd/Connection_secure.h"
#include "logjamd/Server_secure.h"
#include "logjamd/Stage_auth.h"
#include "lj/Exception.h"

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
}

#include <algorithm>
#include <mutex>
#include <thread>

namespace
{
    struct Buffer
    {
        char* buffer;
        int32_t offset;
        int32_t size;
        bool header;
        inline int32_t avail() const { return size - offset; };
        inline char* pos() { return buffer + offset; };
        inline void reset(size_t sz) {
            if (buffer)
            {
                delete[] buffer;
            }
            buffer = NULL;
            if (sz)
            {
                buffer = new char[sz];
            }
            offset = 0;
            size = sz;
            header = true;
        };
    };

    struct State
    {
        State(logjamd::Connection_secure* connection, BIO* io)
                : connection_(connection), io_(io)
        {
            in_.reset(4096);
            out_.reset(0);
            read_.reset(4);
            write_.reset(0);
        }
        Buffer in_;
        Buffer out_;
        Buffer read_;
        Buffer write_;
        logjamd::Connection_secure* connection_;
        BIO* io_;
    };

    class Server_io
    {
    public:
        Server_io() : stop_(false), mutex_(), clients_()
        {
        }
        Server_io(const Server_io& orig) = delete;
        Server_io& operator=(const Server_io& orig) = delete;
        ~Server_io()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto iter = clients_.begin();
                clients_.end() != iter;
                ++iter)
            {
                delete *iter;
            }
        }

        int populate_sets(fd_set* rs, fd_set* ws, std::map<int, State*>& cache)
        {
            FD_ZERO(rs);
            FD_ZERO(ws);
            int max_fd = 0;

            std::lock_guard<std::mutex> _(mutex_);
            for (auto iter = clients_.begin();
                clients_.end() != iter;
                ++iter)
            {
                int fd;
                BIO_get_fd((*iter)->io_, &fd);

                if ((*iter)->connection_->writing())
                {
                    FD_SET(fd, ws);
                }
                else
                {
                    FD_SET(fd, rs);
                }

                max_fd = std::max(max_fd, fd);
                cache[fd] = *iter;
            }
            return max_fd;
        }

        template <class F>
        void perform_io(F func, Buffer& buffer, State* state,
                std::list<State*>& remove)
        {
            int nbytes = func(state->io_, buffer.pos(), buffer.avail());
            if (nbytes <= 0)
            {
                // deal with temporary errors
                if (!BIO_should_retry(state->io_))
                {
                    // remove the socket from future loops.
                    remove.push_back(state);
                }
            }
            else 
            {
                buffer.offset += nbytes;
            }
        }

        void select()
        {
            fd_set rs;
            fd_set ws;
            std::map<int, State*> cache;

            // Call into a smaller function for locking.
            int mx = populate_sets(&rs, &ws, cache);

            // Perform select on the open sockets.
            if (-1 == ::select(mx + 1, &rs, &ws, NULL, NULL))
            {
                throw LJ__Exception("Unable to perform select.");
            }

            // Loop over the known sockets and see if they are
            // ready for reading/writing.
            std::list<State*> remove;
            for (auto iter = cache.begin();
                cache.end() != iter;
                ++iter)
            {
                State* state = (*iter).second;
                if (FD_ISSET((*iter).first, &rs))
                {
                    perform_io<int (*)(BIO*, void*, int)>(BIO_read,
                            state->in_, state, remove);
                }
                else if (FD_ISSET((*iter).first, &ws))
                {
                    perform_io<int (*)(BIO*, const void*, int)>(BIO_write,
                            state->out_, state, remove);
                }
            }

            for (auto iter = remove.begin();
                remove.end() != iter;
                ++iter)
            {
                release_client(*iter);
            }
        }

        void operator()()
        {
            while (!stop_)
            {
                select();
            }
        }

        void manage_client(State* state)
        {
            std::lock_guard<std::mutex> _(mutex_);
            clients_.push_back(state);
        }

        void release_client(const State* state)
        {
            std::lock_guard<std::mutex> _(mutex_);
            clients_.remove_if([&state](State* value) -> bool
            {
                return state == value;
            });
            delete state;
        }

        void stop()
        {
            stop_ = true;
        }
    private:
        bool stop_;
        std::mutex mutex_;
        std::list<State*> clients_;
    };
}; // namespace (anonymous)

namespace logjamd
{
    Server_secure::Server_secure(lj::Document* config)
            : logjamd::Server(config), io_(NULL), running_(false), client_io_thread_(NULL)
    {
    }
    Server_secure::~Server_secure()
    {
        if (io_)
        {
            BIO_free(io_);
        }

        if (client_io_thread_)
        {
            client_io_thread_->join();
        }
    }
    void Server_secure::startup()
    {
        std::string listen(lj::bson::as_string(cfg().get("server/listen")));
        const lj::bson::Node& cluster = cfg().get("server/cluster");

        // start peer connections.
        for (auto iter = cluster.to_vector().begin();
            cluster.to_vector().end() != iter;
            ++iter)
        {
            std::string peer(lj::bson::as_string(*(*iter)));
            char* peer_address = new char[peer.size() + 1];
            memcpy(peer_address, peer.c_str(), peer.size() + 1);
            delete[] peer_address;
        }

        // Start port listening.
        char* host_port = new char[listen.size() + 1];
        memcpy(host_port, listen.c_str(), listen.size() + 1);
        io_ = BIO_new_accept(host_port);

        if (!io_)
        {
            ERR_print_errors_fp(stderr);
            throw LJ__Exception("Unable to allocate IO object.");
        }

        if (BIO_do_accept(io_) <= 0)
        {
            ERR_print_errors_fp(stderr);
            throw LJ__Exception("Unable to setup the listener object.");
        }
    }
    void Server_secure::listen()
    {
        running_ = true;
        while(running_)
        {
            if (BIO_do_accept(io_) <= 0)
            {
                ERR_print_errors_fp(stderr);
                throw LJ__Exception("Unable to accept a connection.");
            }
            lj::Document* client_state = new lj::Document();
            BIO* client_io = BIO_pop(io_);
            logjamd::Connection_secure* client =
                    new logjamd::Connection_secure(this, client_state);
            State* state = new State(client, client_io);
            logjamd::Stage* stage = new logjamd::Stage_auth(client);

            while(stage)
            {
                stage = stage->logic();
            }

            delete client;
            running_ = false;
        }
    }
    void Server_secure::shutdown()
    {
        running_ = false;
    }
};
