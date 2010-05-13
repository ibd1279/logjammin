/*!
 \file logjam_net.h
 \brief Logjam client networking code.
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

#include "Sockets.h"

namespace logjam
{    
    class Send_bytes : public lj::Socket_dispatch {
    public:
        Send_bytes(const char* buffer, int sz);
        virtual ~Send_bytes();
        virtual void set_socket(int sock)
        {
            s_ = sock;
        }
        virtual int socket()
        {
            return s_;
        }
        virtual void set_mode(lj::Socket_dispatch::Socket_mode mode)
        {
            m_ = mode;
        }
        virtual lj::Socket_dispatch::Socket_mode mode()
        {
            return m_;
        }
        virtual bool is_writing()
        {
            return is_w_;
        }
        virtual lj::Socket_dispatch* accept(int socket, char* buffer);
        virtual void read(const char* buffer, int sz);
        virtual const char* write(int* sz);
        virtual void written(int sz);
        virtual void close();
    private:
        bool is_w_;
        int s_;
        lj::Socket_dispatch::Socket_mode m_;
        char* out_;
        int out_offset_;
        int out_sz_;
    };
};