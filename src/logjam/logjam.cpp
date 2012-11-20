/*
 \file logjam.cpp
 \author Jason Watson

 Copyright (c) 2009, Jason Watson
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


#include "logjam/Client_socket.h"
#include "logjam/Network_address_info.h"
#include "logjam/Tls_credentials.h"
#include "logjam/Tls_globals.h"
#include "logjam/Tls_session.h"
#include "lj/Bson.h"
#include "lj/Log.h"
#include "lj/Streambuf_bsd.h"
#include "logjamd/constants.h"
#include "Network_connection.h"

#include <iostream>
#include <unistd.h>


int main(int argc, char * const argv[])
{
    try
    {
        logjam::Tls_globals globals; // this ensures that the deinit is called when the stack unwinds.
        
        // This should take care of all the connection steps.
        std::iostream* io = logjam::client::create_connection("12345", "vson");

        // Perform the authentication.
        lj::bson::Node auth;
        auth.set_child("/method", lj::bson::new_uuid(logjamd::k_auth_method_password));
        auth.set_child("/provider", lj::bson::new_uuid(logjamd::k_auth_provider_local));
        auth.set_child("/data/login", lj::bson::new_string(logjamd::k_user_login_json));
        auth.set_child("/data/password", lj::bson::new_string(logjamd::k_user_password_json));
        
        // Communicate across the secure pipe.
        lj::bson::Node response;
        (*io) << auth;
        (*io).flush();
        (*io) >> response;
        
        // Look to make sure the response was successful.
        if (logjam::client::is_success(response))
        {
            lj::log::out<lj::Info>("Authenticated.");
        }
    }
    catch (const lj::Exception& ex)
    {
        lj::log::out<lj::Error>(ex.str());
    }
}