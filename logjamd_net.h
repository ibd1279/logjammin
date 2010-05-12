/*!
 \file logjamd_net.h
 \brief Logjam Server Networking code.
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

#include <map>
#include <string>

namespace logjamd
{
    class Socket_dispatch {
    public:
        enum Socket_mode
        {
            k_listen,
            k_communicate
        };
        virtual ~Socket_dispatch()
        {
        }
        virtual void set_socket(int) = 0;
        virtual int socket() = 0;
        virtual void set_mode(Socket_mode) = 0;
        virtual Socket_mode mode() = 0;
        virtual bool is_writing() = 0;
        virtual Socket_dispatch* accept(int socket, char*) = 0;
        virtual void read(const char*, int) = 0;
        virtual const char* write(int*) = 0;
        virtual void written(int) = 0;
        virtual void close() = 0;
    };
    
    class Socket_listener {
    public:
        Socket_listener();
        ~Socket_listener();
        void bind_port(int port, Socket_dispatch* dispatch);
        void select();
    private:
        Socket_listener(const Socket_listener&);
        Socket_listener& operator=(const Socket_listener&);
        int populate_sets(fd_set*, fd_set*);
        
        std::map<int, Socket_dispatch*> ud_;
    };
    
    class Service_dispatch : public Socket_dispatch {
    public:
        Service_dispatch();
        virtual ~Service_dispatch();
        virtual void set_socket(int sock)
        {
            s_ = sock;
        }
        virtual int socket()
        {
            return s_;
        }
        virtual void set_mode(Socket_dispatch::Socket_mode mode)
        {
            m_ = mode;
        }
        virtual Socket_dispatch::Socket_mode mode()
        {
            return m_;
        }
        virtual bool is_writing()
        {
            return is_w_;
        }
        virtual Socket_dispatch* accept(int socket, char* buffer);
        virtual void read(const char* buffer, int sz);
        virtual const char* write(int* sz);
        virtual void written(int sz);
        virtual void close();
    private:
        bool is_w_;
        int s_;
        Socket_dispatch::Socket_mode m_;
        std::string ip_;
        int sz_;
        char* out_;
    };
};