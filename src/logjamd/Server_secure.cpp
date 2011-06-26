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
    struct State
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

    struct Connection_secure_state
    {
        State in_;
        State out_;
        State read_;
        State write_;
        Connection_secure* connection_;
        BIO* io_;
    };

    class Server_secure_io
    {
    public:
        Server_secure_io() : stop_(false), mutex_(), clients_()
        {
        }
        Server_secure_io(const Server_secure_io& orig) = delete;
        Server_secure_io& operator=(const Server_secure_io& orig) = delete;
        ~Server_secure_io()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto iter = clients_.begin();
                clients_.end() != iter;
                ++iter)
            {
                delete *iter;
            }
        }

        int populate_sets(fd_set* rs, fd_set* ws)
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

                if ((*iter)->writing())
                {
                    FD_SET(fd, ws);
                }
                else
                {
                    FD_SET(fd, rs);
                }

                max_fd = std::max(max_fd, fd);
            }
            return max_fd;
        }

        void operator()()
        {
            fd_set rs;
            fd_set ws;

            while (!stop_)
            {
                int mx = populate_sets(&rs, &ws);
                if (-1 == ::select(mx + 1, &rs, &ws, NULL, NULL))
                {

                }
            }
        }

        void manage_client(logjamd::Connection_secure* client)
        {
            Connection_secure_state* state = new Connection_secure_state();
            state->connection_ = client;
            std::lock_guard<std::mutex> _(mutex_);
            clients_.push_back(state);
        }

        void release_client(const logjamd::Connection_secure* client)
        {
            std::lock_guard<std::mutex> _(mutex_);
            clients_.remove_if([&client](Connection_secure_state* value) -> bool
            {
                return client == value->connection_;
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
        std::list<logjamd::Connection_secure_state*> clients_;
    };
}; // namespace (anonymous)

namespace logjamd
{
    Server_secure::Server_secure(lj::Document* config)
            : logjamd::Server(config), io_(NULL), running_(false)
    {
    }
    Server_secure::~Server_secure()
    {
        if (io_)
        {
            BIO_free(io_);
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
            logjamd::Connection* client = new logjamd::Connection_secure(this,
                    client_state, client_io);
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
