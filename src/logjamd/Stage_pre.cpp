/*!
 \file Stage_pre.cpp
 \brief Logjam server stage pre connection implementation.
 \author Jason Watson

 Copyright (c) 2014, Jason Watson
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
#include "logjamd/Response.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/Stage_http_adapt.h"
#include "logjamd/constants.h"
#include "logjam/User.h"

#include "lj/Bson.h"
#include "lj/Log.h"
#include <locale>

namespace
{
    const std::string k_bson_mode("bson\n");
    const std::string k_http_get_mode("get /");
    const std::string k_http_post_mode("post ");
    const std::string k_tls_mode("+tls\n");
    const std::string k_peer_mode("peer\n");
    const std::string k_error_unknown_mode("Unknown mode: ");
};

namespace logjamd
{
    std::unique_ptr<logjam::Stage> Stage_pre::logic(logjam::pool::Swimmer& swmr) const
    {
        log("Starting logic.").end();
        char buffer[6];
        std::locale loc;
        for (int h = 0; h < 5; ++h)
        {
            char c;
            swmr.io().get(c);
            buffer[h] = std::tolower(c, loc);
        }
        buffer[5] = '\0';

        if ('\r' == buffer[4])
        {
            buffer[4] = '\n';
        }

        if (k_bson_mode.compare(buffer) == 0)
        {
            log("Using BSON mode.").end();
            swmr.io() << response::new_empty(*this);
            return std::unique_ptr<logjam::Stage>(new Stage_auth());
        }
        else if (k_http_get_mode.compare(buffer) == 0)
        {
            log("Using HTTP get mode.").end();
            swmr.context().node().set_child("http_adapt/method",
                    lj::bson::new_string("get"));
            return std::unique_ptr<logjam::Stage>(new Stage_http_adapt());
        }
        else if (k_http_post_mode.compare(buffer) == 0)
        {
            log("Using HTTP post mode.").end();
            swmr.io().get(); // Consume the prefix slash.
            swmr.context().node().set_child("http_adapt/method",
                    lj::bson::new_string("post"));
            return std::unique_ptr<logjam::Stage>(new Stage_http_adapt());
        }
        else
        {
            std::string mode(buffer, 4);
            log("Unknown mode provided: %s").end(mode);
            swmr.io() << lj::bson::as_string(response::new_error(*this,
                    k_error_unknown_mode + mode));
            return nullptr;
        }
    }

    std::string Stage_pre::name() const
    {
        return std::string("Pre-connection");
    }

    std::unique_ptr<logjam::Stage> Stage_pre::clone() const
    {
        return std::unique_ptr<Stage>(new Stage_pre(*this));
    }
};
