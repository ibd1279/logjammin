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

extern "C" {
#include "lualib.h"
}
#include <sstream>
#include <list>
#include "Bson.h"
#include "Logger.h"
#include "logjamd_net.h"

namespace logjamd
{
    Service_dispatch::Service_dispatch() : is_w_(false), s_(0), m_(k_listen), ip_(), in_(0), in_offset_(0), in_sz_(4), in_post_length_(false), out_(0), out_offset_(0), out_sz_(0), lua_(0)
    {
        in_ = new char[4];
        lua_ = lua_open();
        luaL_openlibs(lua_);
    }
    Service_dispatch::~Service_dispatch()
    {
        if (in_)
        {
            delete[] in_;
        }
        
        if (lua_)
        {
            lua_close(lua_);
        }
        
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
            lj::Bson b;
            b.set_value(lj::k_bson_document, in_);
            
            // XXX one doc at a time mode. needs to be modified to batch.
            logic(b);
            
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
    const char* Service_dispatch::write(int* sz)
    {
        *sz = out_sz_ - out_offset_;
        return out_ + out_offset_;
    }
    void Service_dispatch::written(int sz)
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
    void Service_dispatch::close()
    {
        ::close(s_);
    }
    
    void Service_dispatch::logic(lj::Bson& b)
    {
        std::string cmd = lj::bson_as_string(b.nav("command"));
        
        b.set_child("is_ok", lj::bson_new_boolean(true));
        int error = luaL_loadbuffer(lua_,
                                    cmd.c_str(),
                                    cmd.size(),
                                    ip_.c_str()) || lua_pcall(lua_, 0, 0, 0);
        if (error)
        {
            const char* error_string = lua_tostring(lua_, -1);
            lj::Log::warning.log("Lua error: %s") << error_string << lj::Log::end;
            b.set_child("error", lj::bson_new_string(error_string));
            lua_pop(lua_, 1);
            b.set_child("is_ok", lj::bson_new_boolean(false));
        }
        
        lj::Log::info.log("doc: %s") << bson_as_pretty_string(b, 0) << lj::Log::end;
    }
}; // namespace logjamd
