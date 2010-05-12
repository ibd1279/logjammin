/*!
 \file logjamd_net.cpp
 \brief Logjam Server Networking implementation.
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

#include <sstream>
#include <list>
#include "logjamd_net.h"

namespace logjamd
{
    Service_dispatch::Service_dispatch() : is_w_(false), s_(0), m_(k_listen), ip_(), sz_(0), out_(0)
    {
    }
    Service_dispatch::~Service_dispatch()
    {
        if (out_)
        {
            delete[] out_;
        }
    }
    lj::Socket_dispatch* Service_dispatch::accept(int socket, char* buffer)
    {
        Service_dispatch* sd = new Service_dispatch();
        sd->set_socket(socket);
        sd->set_mode(Socket_dispatch::k_communicate);
        sd->ip_ = buffer;
        return sd;
    }
    void Service_dispatch::read(const char* buffer, int sz)
    {
        out_ = new char[sz];
        memcpy(out_, buffer, sz);
        sz_ = sz;
        is_w_ = true;
    }
    const char* Service_dispatch::write(int* sz)
    {
        *sz = sz_;
        return out_;
    }
    void Service_dispatch::written(int sz)
    {
        sz_ -= sz;
        if (sz_)
        {
            memmove(out_, out_ + sz, sz_);
        }
        else
        {
            is_w_ = false;
        }
    }
    void Service_dispatch::close()
    {
        ::close(s_);
    }
}; // namespace logjamd
