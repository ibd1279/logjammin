/*!
 \file Stage_json_adapt.cpp
 \brief Logjam server stage for converting telnet json input into bson input.
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

#include "logjamd/Stage_json_adapt.h"
#include "logjamd/Stage_auth.h"

namespace logjamd
{
    Stage_json_adapt::Stage_json_adapt(Connection* connection) :
            Stage(connection),
            pipe_(),
            faux_connection_(connection, new std::iostream(&pipe_)),
            real_stage_(new Stage_auth(&faux_connection_))
    {
    }

    Stage_json_adapt::~Stage_json_adapt()
    {
        if (real_stage_)
        {
            delete real_stage_;
            real_stage_ = NULL;
        }
    }

    Stage* Stage_json_adapt::logic()
    {
        // Deal with "un-auth" authentication.

        // Needs to read bytes from the real connection and write
        // them into the pipe. then call logic on the real_stage_,
        // read the bytes from the pipe and write them to the real
        // connection.
        return this;
    }

    std::string Stage_json_adapt::name()
    {
        if (real_stage_)
        {
            return real_stage_->name();
        }
        return std::string("JSON-Adapter");
    }
};

