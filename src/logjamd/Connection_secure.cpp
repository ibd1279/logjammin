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

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
}


namespace logjamd
{
    Connection_secure::Connection_secure(logjamd::Server* server,
            lj::Document* state, ::BIO* io) : logjamd::Connection(server, state), io_(io)
    {
    }
    Connection_secure::~Connection_secure()
    {
        if (io_)
        {
            BIO_free(io_);
        }
    }
    lj::bson::Node* Connection_secure::read()
    {
        uint8_t length_buffer[4];
        uint32_t found = 0;
        while(4 > found)
        {
            int tmp = BIO_read(io_, length_buffer + found, 4 - found);
            if (0 >= tmp)
            {
                throw LJ__Exception("Unable to read from Connection.");
            }
            found += tmp;
        }

        uint32_t length = *((uint32_t*)length_buffer);
        uint8_t* document_buffer = new uint8_t[length];
        found = 0;
        while(length > found)
        {
            int tmp = BIO_read(io_, length_buffer + found, length - found);
            if (0 >= tmp)
            {
                delete[] document_buffer;
                throw LJ__Exception("Unable to read from Connection.");
            }
            found += tmp;
        }
        lj::bson::Node* ptr = new lj::bson::Node(lj::bson::Type::k_document, document_buffer);
        delete[] document_buffer;
        return ptr;
    }
    void Connection_secure::write(const lj::bson::Node& data)
    {
        size_t sz = data.size();
        uint8_t* buffer = data.to_binary();
        uint32_t sent = 0;
        while(sz > sent)
        {
            int tmp = BIO_write(io_, buffer, sz);
            if (0 > tmp)
            {
                delete[] buffer;
                throw LJ__Exception("Unable to write to Connection.");
            }
            sent += tmp;
        }
        delete[] buffer;
    }
};
