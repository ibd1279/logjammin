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
            lj::Document* state) : logjamd::Connection(server, state)
    {
    }
    Connection_secure::~Connection_secure()
    {
    }
    lj::bson::Node* Connection_secure::read()
    {
        std::lock_guard<std::mutex> _(mutex_);
        if (read_queue_.empty())
        {
            return NULL;
        }
        lj::bson::Node* node = read_queue_.front();
        read_queue_.pop();
        return node;
    }
    void Connection_secure::write(const lj::bson::Node& data)
    {
        std::lock_guard<std::mutex> _(mutex_);
        write_queue_.push(new lj::bson::Node(data));
    }
    void Connection_secure::enqueue(lj::bson::Node* node)
    {
        std::lock_guard<std::mutex> _(mutex_);
        read_queue_.push(node);
    }

    lj::bson::Node* Connection_secure::dequeue()
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

    bool Connection_secure::writing()
    {
        std::lock_guard<std::mutex> _(mutex_);
        return write_queue_.empty();
    }
};
