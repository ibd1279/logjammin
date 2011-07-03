#pragma once
/*!
 \file logjamd/Connection.h
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

#include "lj/Bson.h"
#include "lj/Document.h"
#include "logjamd/Server.h"

#include <istream>

namespace logjamd
{
    class User;
    class Stage_auth;

    class Connection {
    public:
        friend class Stage_auth;
        Connection(logjamd::Server* server,
                lj::Document* state,
                std::iostream* stream) :
                server_(server),
                state_(state),
                stream_(stream),
                user_(NULL)
        {
        }

        //! Destructor.
        virtual ~Connection()
        {
            if (state_)
            {
                delete state_;
            }

            if (stream_)
            {
                stream_->flush();
                delete stream_;
            }
        }
        virtual void start() = 0;
        virtual logjamd::Server& server()
        {
            return *server_;
        }
        virtual lj::Document& state()
        {
            return *state_;
        }
        virtual std::iostream& io()
        {
            return *stream_;
        }
        virtual const User* user()
        {
            return user_;
        }
    protected:
        virtual void user(logjamd::User* u)
        {
            user_ = u;
        }
    private:
        logjamd::Server* server_;
        lj::Document* state_;
        std::iostream* stream_;
        logjamd::User* user_;
    };
};


