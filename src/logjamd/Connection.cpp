/*!
 \file Connection.cpp
 \brief Logjam server connection to a client implementation.
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

#include "logjamd/Connection.h"

#include "logjamd/Stage_auth.h"
#include "lj/Bson.h"
#include "lj/Logger.h"
#include "lj/Time_tracker.h"

#include <cstring>
#include <list>
#include <string>
#include <sstream>
#include <sys/time.h>

namespace logjamd
{
    Connection::Connection(const std::string& client_ip,
                           lua_State* client_lua,
                           const lj::Bson* server_config,
                           const std::string& data_directory) :
            in_(0), in_offset_(0), in_sz_(4), in_post_length_(false),
            ip_(client_ip), lua_(client_lua), server_config_(server_config),
            data_dir_(data_directory), stage_(0)
    {
        stage_ = new Stage_auth();
    }
    
    Connection::~Connection()
    {
        if (in_)
        {
            delete[] in_;
        }
        if (stage_)
        {
            delete stage_;
        }
    }
    
    lj::Socket_dispatch* Connection::accept(int socket, const std::string& buffer)
    {
        return 0;
    }
    
    void Connection::read(const char* buffer, int sz)
    {
        if (!in_)
        {
            in_ = new char[4];
            in_offset_ = 0;
            in_sz_ = 4;
            in_post_length_ = false;
        }
        
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
        
        // if we hit the end of the document, execute.
        if (in_offset_ == in_sz_)
        {
            lj::Bson b(lj::Bson::k_document, in_);
            
            stage_ = stage_->logic(b, *this);

            if (!stage_)
            {
                close();
            }
            
            delete[] in_;
            in_ = 0;
        }
        
        // Check if we have more to read and recurse.
        if (sz - read_offset)
        {
            read(buffer + read_offset, sz - read_offset);
        }
    }
    
}; // namespace logjamd
