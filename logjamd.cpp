/*!
 \file logjamd.cpp
 \brief Logjam Server Executable
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

#include <cerrno>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "Logger.h"


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char * const argv[]) {
    lj::Log::debug.disable();
    lj::Log::info.enable();

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    struct addrinfo *info;
    int status;
    if ((status = getaddrinfo(NULL, "27754", &hints, &info)))
    {
        lj::Log::emergency.log("Unable to get address info: [%s].") << gai_strerror(status) << lj::Log::end;
        return 1;
    }
    
    int sock;
    struct addrinfo* iter;
    for (iter = info; iter != 0; iter = iter->ai_next)
    {
        if (-1 == (sock = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)))
        {
            sock = NULL;
            lj::Log::warning.log("Unable to open socket: [%d][%s].") << errno << strerror(errno) << lj::Log::end;
            continue;
        }
        
        int opt_on = 1;
        if (-1 == setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_on, sizeof(int)))
        {
            lj::Log::emergency.log("Unable to set options on socket: [%d][%s].") << errno << strerror(errno) << lj::Log::end;
            return 1;
        }
        
        if (-1 == bind(sock, iter->ai_addr, iter->ai_addrlen))
        {
            close(sock);
            lj::Log::emergency.log("Unable to bind: [%d][%s].") << errno << strerror(errno) << lj::Log::end;
            continue;
        }
        
        break;
    }
    
    if (!sock)
    {
        lj::Log::emergency.log("Unable to bind to any port.") << lj::Log::end;
        return 2;
    }
    
    freeaddrinfo(info);
    
    if (-1 == listen(sock, 5))
    {
        lj::Log::emergency.log("Unable to listen: [%d][%s].") << errno << strerror(errno) << lj::Log::end;
        return 2;
    }
    
    while(true)
    {
        struct sockaddr_storage client_addr;
        socklen_t sz = sizeof(struct sockaddr_storage);
        int client_sock = accept(sock, (struct sockaddr *)&client_addr, &sz);
        if (client_sock == -1)
        {
            lj::Log::info.log("Bad Accept: [%d][%s].") << errno << strerror(errno) << lj::Log::end;
            continue;
        }
        
        char s[INET6_ADDRSTRLEN];
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, INET6_ADDRSTRLEN);
        lj::Log::info.log("Got connection from %s") << s << lj::Log::end;
        
        if (!fork())
        {
            close(sock);
            if (-1 == send(client_sock, "Hello, world!", 13, 0))
            {
                lj::Log::info.log("Bad send: [%d][%s].") << errno << strerror(errno) << lj::Log::end;
            }
            close(client_sock);
            return 0;
        }
    }
    return 0;
}