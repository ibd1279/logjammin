/*!
 \file Stage_pre.cpp
 \brief Logjam server stage pre connection implementation.
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

#include "logjamd/Stage_pre.h"
#include "logjamd/Connection.h"

#include "lj/Bson.h"
#include "lj/Log.h"
#include <locale>

namespace
{
    const std::string k_bson_mode("bson\n");
    const std::string k_json_mode("json\n");
    const std::string k_http_mode("http ");
    const std::string k_error_unknown_mode("Unknown mode: ");
};

namespace logjamd
{
    Stage_pre::Stage_pre(logjamd::Connection* connection)
            : logjamd::Stage(connection)
    {
    }

    Stage_pre::~Stage_pre()
    {
    }

    Stage* Stage_pre::logic()
    {
        char buffer[6];
        std::locale loc;
        for (int h = 0; h < 5; ++h)
        {
            char c;
            conn()->io().get(c);
            buffer[h] = std::tolower(c, loc);
        }
        buffer[5] = '\0';

        if (k_bson_mode.compare(buffer) == 0)
        {
            log("Using BSON mode.") << lj::Log::end;
        }
        else if (k_json_mode.compare(buffer) == 0)
        {
            log("Using json mode.") << lj::Log::end;
        }
        else if (k_http_mode.compare(buffer) == 0)
        {
            log("Using HTTP mode.") << lj::Log::end;
        }
        else
        {
            std::string mode(buffer, 4);
            log("Unknown mode provided: %s") << mode << lj::Log::end;
            conn()->io() << lj::bson::as_string(error_response(k_error_unknown_mode + mode));
            return NULL;
        }
    }

    std::string Stage_pre::name()
    {
        return std::string("Pre-connection");
    }
};


