#pragma once
/*!
 \file logjamd/Connection_connection.h
 \brief Logjam server connection to a client definition.
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

#include "logjamd/Connection.h"
#include "crypto++/secblock.h"
#include <map>
#include <string>
#include <thread>

typedef struct bio_st BIO;
namespace logjamd
{
    class Connection_secure : public logjamd::Connection
    {
    public:
        Connection_secure(logjamd::Server* server,
                lj::bson::Node* state,
                std::iostream* stream);
        virtual ~Connection_secure();
        virtual void start();
        virtual void operator()();
        virtual bool secure()
        {
            return secure_;
        }
        virtual void set_crypto_key(const std::string& identifier,
                const void* key,
                int sz);
        virtual const void* get_crypto_key(const std::string& identifier,
                int* sz);
    protected:
        inline std::thread& thread()
        {
            return *thread_;
        }
    private:
        std::thread* thread_;
        bool secure_;
        std::map<std::string, CryptoPP::SecBlock<uint8_t>* > keys_;
    };
};


