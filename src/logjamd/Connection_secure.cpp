/*!
 \file logjamd/Connection_secure.cpp
 \brief Logjam server connection to a client implementation.
 \author Jason Watson

 Copyright (c) 2010, Jason Watson
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
#include "logjamd/Stage.h"
#include "logjamd/Stage_pre.h"
#include "logjam/Tls_credentials.h"
#include "logjam/Tls_session.h"
#include "lj/Streambuf_bsd.h"
#include "Server_secure.h"
#include "logjam/Network_connection.h"

#include <algorithm>
#include <assert.h>

namespace logjamd
{
    Connection_secure::Connection_secure(
            logjamd::Server_secure* server,
            lj::bson::Node* state,
            logjam::Network_connection&& connection,
            std::iostream* insecure_stream) :
            logjamd::Connection(server, state, insecure_stream),
            server_(server),
            connection_(std::move(connection)),
            thread_(nullptr),
            secure_(false),
            keys_()
    {
    }

    Connection_secure::~Connection_secure()
    {
        if (thread_)
        {
            thread_->join();
            delete thread_;
        }

        close();
    }

    void Connection_secure::start()
    {
        thread_ = new lj::Thread();
        thread_->run(this);
    }

    void Connection_secure::make_secure()
    {
        lj::log::out<lj::Debug>("Attempting to make the connection secure.");
        assert(!secure_);

        // Get the session for communication.
        std::unique_ptr<Server_secure::Session> session(
                server_->new_session(connection_.socket()));

        // Make sure we have sent all the unencrypted data.
        io().flush();

        // Perform the SSL handshake.
        session->handshake();

        // Create the new buffer to replace the old buffer.
        std::streambuf* old_buffer = io().rdbuf();
        io().rdbuf(new lj::Streambuf_bsd<Server_secure::Session>(
                session.release(), 8192, 8192));
        delete old_buffer;

        // At this point, things should be secure.
        secure_ = true;
    }

    void Connection_secure::close()
    {
        lj::log::out<lj::Debug>("Closing the connection.");
                
        // Connection::close() calls flush, so we clean up the rdbuf AFTER
        // it has been called. We also have to close up the TLS session properly
        // if the connection is secure.
        std::streambuf* buffer = io().rdbuf();
        this->logjamd::Connection::close();
        if (secure())
        {
            lj::Streambuf_bsd<Server_secure::Session>* secure_buffer =
                    dynamic_cast<lj::Streambuf_bsd<Server_secure::Session>*>(buffer);
            assert(nullptr != secure_buffer);
            Server_secure::Session& session = secure_buffer->medium();
            
            // TODO shutdown the TLS connection.
            lj::log::out<lj::Critical>("TODO TLS shutdown code is not yet implemented.");
        }
        delete buffer;
        
        // Now we close up the actual network connection. This is not handled
        // by the buffers because we use different buffers on the same socket
        // at different points in the connection.
        connection_.close();
    }

    void Connection_secure::run()
    {
        Stage* stage = new Stage_pre(this);

        while (stage)
        {
            try
            {
                Stage* new_stage = stage->logic();
                io().flush();

                // protect against any stages that return themselves.
                if (new_stage != stage)
                {
                    delete stage;
                    stage = new_stage;
                }
            }
            catch (const lj::Exception& ex)
            {
                delete stage;
                stage = nullptr;
            }
        }
        lj::log::out<lj::Debug>("Connection Thread Exited.");
    }

    void Connection_secure::cleanup()
    {
        lj::log::out<lj::Debug>("Detaching connection from the server.");
        server().detach(this);
        // perform any clean up work here.
        delete this;
    }

    void Connection_secure::set_crypto_key(const std::string& identifier,
            const void* key,
            int sz)
    {
        std::unique_ptr<uint8_t[], lj::Wiper<uint8_t[]> > key_ptr(new uint8_t[sz]);
        key_ptr.get_deleter().set_count(sz);
        memcpy(key_ptr.get(), key, sz);
        keys_[identifier] = std::move(key_ptr);
    }

    const void* Connection_secure::get_crypto_key(
            const std::string& identifier,
            int* sz)
    {
        auto iter = keys_.find(identifier);
        if(keys_.end() != iter)
        {
            *sz = (*iter).second.get_deleter().count();
            return (*iter).second.get();
        }
        else
        {
            *sz = 0;
            return nullptr;
        }
    }
}; // namespace logjamd
