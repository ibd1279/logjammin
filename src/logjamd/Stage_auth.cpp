/*!
 \file Stage_auth.cpp
 \brief Logjam server client authentication implementation.
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

#include "logjamd/Stage_auth.h"
#include "logjamd/Stage_execute.h"

#include "lj/Logger.h"

#include <string>

namespace logjamd
{
    Stage_auth::Stage_auth() : Stage::Stage(), attempt_(0)
    {
    }

    Stage_auth::~Stage_auth()
    {
    }

    Stage* Stage_auth::logic(lj::Bson& request, Connection& connection)
    {
        // increment the authentication attempt count.
        attempt_++;

        lj::Log::info.log("Login attempt %d") << attempt_ << lj::Log::end;

        // Create response document
        lj::Bson response;
        response.set_child("is_ok", lj::bson_new_boolean(false));

        // parse document.
        std::string method(lj::bson_as_string(request.nav("method")));
        std::string provider(lj::bson_as_string(request.nav("provider")));
        std::string identity(lj::bson_as_string(request.nav("identity")));
        std::string token(lj::bson_as_string(request.nav("token")));

        // perform authentication
        lj::Log::info.log("  method %s") << method << lj::Log::end;
        if (method.compare("fake") == 0)
        {
            lj::Log::info.log("  provider %s") << provider << lj::Log::end;
            if (provider.compare("local") == 0)
            {
                lj::Log::info.log("  identity %s") << identity << lj::Log::end;
                if (identity.compare("admin") == 0)
                {
                    lj::Log::info.log("  token %s") << token << lj::Log::end;
                    if (token.compare("insecure") == 0)
                    {
                        lj::Log::info.log("login successful") << lj::Log::end;

                        // Create a successful response
                        response.set_child("is_ok", lj::bson_new_boolean(true));

                        // send response.
                        char* buffer = response.to_binary();
                        connection.add_bytes(buffer, response.size());
                        delete[] buffer;
                        connection.set_writing(true);

                        // Return the next processor.
                        return new Stage_execute();
                    }
                }
            }
        }
        lj::Log::info.log("login failure.") << lj::Log::end;

        // send response.
        char* buffer = response.to_binary();
        connection.add_bytes(buffer, response.size());
        delete[] buffer;
        connection.set_writing(true);

        // Login failed, so bounce it back.
        if (attempt_ > 3)
        {
            // too many attempts, close connection.
            return 0;
        }

        // return this to ensure we auth on the next command.
        return this;
    }
};

