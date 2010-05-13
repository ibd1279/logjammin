/*!
 \file logjam_net.cpp
 \brief Logjam client networking implementation.
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
#include "Logger.h"
#include "logjam_net.h"

namespace logjam
{
    Send_bytes::Send_bytes() : is_w_(true), s_(0), m_(k_listen), out_(0), out_offset_(0), out_sz_(0)
    {
    }
    
    Send_bytes::~Send_bytes()
    {
        if (out_)
        {
            delete[] out_;
        }
    }
    
    lj::Socket_dispatch* Send_bytes::accept(int socket, char* buffer)
    {
        return NULL;
    }
    
    void Send_bytes::read(const char* buffer, int sz)
    {
    }
    
    const char* Send_bytes::write(int* sz)
    {
        *sz = out_sz_ - out_offset_;
        return out_ + out_offset_;
    }
    void Send_bytes::written(int sz)
    {
        out_offset_ += sz;
        if (out_offset_ == out_sz_)
        {
            delete[] out_;
            out_sz_ = 0;
            out_offset_ = 0;
            out_ = 0;
            is_w_ = false;
        }
    }
    void Send_bytes::close()
    {
        ::close(s_);
    }
    void Send_bytes::add_bytes(const char* buffer, int sz)
    {
        char* ptr = new char[sz + out_sz_];
        if (out_)
        {
            memcpy(ptr, out_, out_sz_);
            delete[] out_;
        }
        memcpy(ptr + out_sz_, buffer, sz);
        out_ = ptr;
        out_sz_ += sz;
        is_w_ = true;
    }
}; // namespace logjamd
