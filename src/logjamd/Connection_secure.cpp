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
#include "logjamd/Stage.h"
#include "logjamd/Stage_auth.h"

#include <algorithm>

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
}


namespace logjamd
{
    Connection_secure::Connection_secure(
            logjamd::Server* server,
            lj::bson::Node* state,
            std::iostream* stream) :
            logjamd::Connection(server, state, stream),
            thread_(NULL)
    {
    }

    Connection_secure::~Connection_secure()
    {
        if (thread_)
        {
            thread_->join();
            delete thread_;
        }
    }

    void Connection_secure::start()
    {
        thread_ = new std::thread(*this);
    }

    void Connection_secure::operator()()
    {
        Stage* stage = new Stage_auth(this);

        while (stage)
        {
            try
            {
                Stage* new_stage = stage->logic();

                if (new_stage != stage)
                {
                    delete stage;
                    stage = new_stage;
                }
            }
            catch (const lj::Exception& ex)
            {
                delete stage;
                stage = NULL;
            }
        }

        // Close the connection.
    }
}; // namespace logjamd
