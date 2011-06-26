/*!
 \file logjamd/Connection_connection.cpp
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

#include <algorithm>

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
}


namespace logjamd
{
    Connection_secure::Connection_secure(logjamd::Server* server,
            lj::Document* state, ::BIO* io) : logjamd::Connection(server, state), io_(io), read_{NULL, 0, 0, true}, in_{NULL, 0, 0, true}
    {
    }
    Connection_secure::~Connection_secure()
    {
        if (io_)
        {
            BIO_free(io_);
        }
        read_.reset(0);
        in_.reset(0);
    }
    lj::bson::Node* Connection_secure::read()
    {
        lj::bson::Node* node = NULL;
        while (!node)
        {
            // initial setup logic
            if (!read_.buffer)
            {
                read_.reset(4);
            }
            if (!in_.buffer)
            {
                in_.reset(4096);
            }

            // read as much as we can.
            int ret = BIO_read(io_, in_.pos(), in_.avail());
            if (ret <= 0)
            {
                // deal with errors
                if (BIO_should_retry(io_))
                {
                    //select wait?
                    continue;
                }
                throw LJ__Exception("Unable to read from connection.");
            }
            in_.offset += ret;

            if (read_.header)
            {
                // process the header.
                if (read_.offset < read_.size)
                {
                    // we still need to fill the length
                    int len = std::min<int>(in_.avail(), read_.avail());
                    memcpy(read_.pos(), in_.pos(), len);
                    read_.offset += len;
                    in_.offset += len;
                }

                if (read_.avail() == 0)
                {
                    // We are done with the header, resize the buffer for
                    // the message.
                    read_.reset(*reinterpret_cast<int32_t*>(read_.buffer));
                    *reinterpret_cast<int32_t*>(read_.buffer) = read_.size;
                    read_.offset = 4;
                    read_.header = false;
                }
            }

            if (!read_.header)
            {
                // read the body until we have the completed message.
                int len = std::min<int>(in_.avail(), read_.avail());
                memcpy(read_.pos(), in_.pos(), len);
                read_.offset += len;
                in_.offset += len;
            }

            if (in_.avail() == 0)
            {
                // the input buffer is empty, reset it.
                in_.reset(0);
            }

            if (read_.avail() == 0)
            {
                // we have a completed message. convert to a document to return.
                node = new lj::bson::Node(lj::bson::Type::k_document,
                        reinterpret_cast<uint8_t*>(read_.buffer));
                read_.reset(0);
            }
        }
        return node;
    }
    void Connection_secure::write(const lj::bson::Node& data)
    {
        std::lock_guard<std::mutex> _(mutex_);
        write_queue_.push(new lj::bson::Node(data));
    }
    void Connection_secure::enque(lj::bson::Node* node)
    {
        std::lock_guard<std::mutex> _(mutex_);
        read_queue_.push(node);
    }

    lj::bson::Node* Connection_secure::deque()
    {
        std::lock_guard<std::mutex> _(mutex_);
        if (write_queue_.empty())
        {
            return NULL;
        }
        lj::bson::Node* node = write_queue_.front();
        write_queue_.pop();
        return node;
    }

    bool Connection_secure::writing() const
    {
        std::lock_guard<std::mutex> _(mutex_);
        return write_queue_.empty();
    }
};
