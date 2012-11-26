#pragma once
/*
 \file logjam/Network_address_info.h
 \brief Logjam Network Address Info header.
 \author Jason Watson

 Copyright (c) 2012, Jason Watson
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

#include "logjam/Network_connection.h"
#include "lj/Bson.h"
#include <iostream>

namespace logjam
{
    namespace client
    {
        //! Check to see if a response was successful.
        bool is_success(const lj::bson::Node& response);
        
        //! Create a connection object.
        /*!
         Creates a fully connected Network_connection (BSD socket) to the target.
         If the target resolves as several network addresses, each name is
         tried.
         \todo This does not currently support any of the TLS authentication
         mechanisms. When replication is implemented, it may be necessary to
         split the server off to use a different connection type.
         \param target_host The \c "hostname:port" of the system to connect to.
         \param target_mode The mode to engage once connected (bson, json, etc.)
         \throws lj::Exception if the connection could not be established.
         */
        std::iostream* create_connection(const std::string& target_host,
                const std::string& target_mode);
        
    };
};