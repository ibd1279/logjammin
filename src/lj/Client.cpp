/*!
 \file Client.cpp
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

#include "lj/Client.h"

#include "lj/Logger.h"
#include "lj/Bson.h"

#include <sstream>
#include <list>

namespace lj
{
    Client::Client() : in_(0), in_offset_(0), in_sz_(4), in_post_length_(false), response_(0)
    {
        in_ = new char[4];
    }
    
    Client::~Client()
    {
        if (in_)
        {
            delete[] in_;
        }
        
        if (response_)
        {
            delete response_;
        }
    }
    
    lj::Socket_dispatch* Client::accept(int socket, char* buffer)
    {
        return NULL;
    }
    
    void Client::read(const char* buffer, int sz)
    {
        int read_offset = 0;
        if (!in_post_length_)
        {
            if (in_offset_ < in_sz_)
            {
                int nbytes = (in_sz_ - in_offset_ > sz - read_offset) ? sz - read_offset : in_sz_ - in_offset_;
                memcpy(in_ + in_offset_, buffer, nbytes);
                in_offset_ += nbytes;
                read_offset += nbytes;
            }
            
            if (in_offset_ == in_sz_)
            {
                char* old = in_;
                in_sz_ = *reinterpret_cast<int32_t*>(in_);
                in_ = new char[in_sz_];
                memcpy(in_, old, 4);
                delete[] old;
                in_post_length_ = true;
            }
        }
        
        if (in_post_length_)
        {
            int nbytes = (in_sz_ - in_offset_ > sz - read_offset) ? sz - read_offset : in_sz_ - in_offset_;
            memcpy(in_ + in_offset_, buffer + read_offset, nbytes);
            in_offset_ += nbytes;
            read_offset += nbytes;
        }
        
        if (in_offset_ == in_sz_)
        {
            if (response_)
            {
                delete response_;
            }
            response_ = new lj::Bson(lj::Bson::k_document, in_);
            
            delete[] in_;
            in_ = new char[4];
            in_offset_ = 0;
            in_sz_ = 4;
            in_post_length_ = false;
        }
        
        if (sz - read_offset)
        {
            read(buffer + read_offset, sz - read_offset);
        }
    }
    
    lj::Bson* Client::response()
    {
        return response_;
    }
    
    void Client::clear()
    {
        if (response_)
        {
            delete response_;
            response_ = 0;
        }
    }    
}; // namespace logjamd
