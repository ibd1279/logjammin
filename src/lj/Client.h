#pragma once
/*!
 \file Client.h
 \brief LJ client header.
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

#include "lj/Sockets.h"

namespace lj
{
    class Bson;
    
    //! Logjam client Socket_dispatch implementation.
    class Client : public Socket_dispatch {
    public:
        //! Destructor
        virtual ~Client();
        
        //! Get the response object from the server.
        /*!
         \return The server response.
         */
        lj::Bson* response();
        
        //! Remove the response object.
        void clear();
        
        //! Send a command to the server.
        /*!
         \param cmd The command to send.
         */
        lj::Bson* send_command(const std::string& cmd);
        
        //! Send a command to the server.
        /*!
         \param cmd The command to send.
         */
        lj::Bson* send_command(const lj::Bson* cmd);
        
        //! Connect to a client.
        /*!
         \param host The server host.
         \param port The server port.
         \return The connected client.
         */
        static lj::Client* connect(const std::string host, int port);
    protected:
        //! Constructor
        Client();
        virtual Socket_dispatch* accept(int socket, char* buffer);
        virtual void read(const char* buffer, int sz);
    private:
        char * in_;
        int in_offset_;
        int in_sz_;
        bool in_post_length_;
        lj::Bson* response_;
    };
};