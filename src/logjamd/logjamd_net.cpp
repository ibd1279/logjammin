/*!
 \file logjamd_net.cpp
 \brief Logjam server networking implementation.
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

#include "logjamd/logjamd_net.h"

#include "logjamd/logjamd_lua.h"
#include "logjamd/Lua_bson.h"
#include "lj/Bson.h"
#include "lj/Logger.h"
#include "lj/Time_tracker.h"

extern "C" {
#include "lualib.h"
}
#include <sstream>
#include <list>
#include <sys/time.h>

namespace logjamd
{
    Service_dispatch::Service_dispatch() : ip_(), in_(0), in_offset_(0), in_sz_(4), in_post_length_(false), lua_(0)
    {
    }
    
    Service_dispatch::~Service_dispatch()
    {
        if (in_)
        {
            delete[] in_;
        }
        
        if (lua_ && Socket_dispatch::k_listen == mode())
        {
            lua_close(lua_);
        }        
    }
    
    lj::Socket_dispatch* Service_dispatch::accept(int socket, char* buffer)
    {
        if (!lua_ && Socket_dispatch::k_listen == mode())
        {
            lua_ = luaL_newstate();
            luaL_openlibs(lua_);
            logjam_lua_init(lua_);
        }
        
        logjam_lua_init_connection(lua_, buffer);
        lua_pop(lua_, 1);
        Service_dispatch* sd = new Service_dispatch();
        sd->set_socket(socket);
        sd->set_mode(Socket_dispatch::k_communicate);
        sd->ip_ = buffer;
        sd->lua_ = lua_;
        return sd;
    }
    
    void Service_dispatch::read(const char* buffer, int sz)
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
        
        if (in_offset_ == in_sz_)
        {
            lj::Bson b;
            b.set_value(lj::Bson::k_document, in_);
            
            logic(b);
            
            delete[] in_;
            in_ = 0;
        }
        
        if (sz - read_offset)
        {
            read(buffer + read_offset, sz - read_offset);
        }
    }
    
    void Service_dispatch::logic(lj::Bson& b)
    {
        lj::Time_tracker timer;
        timer.start();
        
        std::string cmd = lj::bson_as_string(b.nav("command"));
        
        // Create the thread.
        lua_State *L = lua_newthread(lua_);
        
        // Load the closure.
        Lua_bson wrapped_node(&b, false);
        Lunar<Lua_bson>::push(L, &wrapped_node, false);
        lua_setglobal(L, "response");
        luaL_loadbuffer(L,
                        cmd.c_str(),
                        cmd.size(),
                        ip_.c_str());
        
        // Hide the global environment.
        logjam_lua_init_connection(L, ip_);
        lua_setfenv(L, -2);
        
        int error;
        while (true)
        {
            error = lua_resume(L, 0);
            if (LUA_YIELD != error)
            {
                // force yields to return instantly.
                // this is incase I decide to do something more co-operative later.
                break;
            }
        }
        
        if (error)
        {
            const char* error_string = lua_tostring(L, -1);
            lj::Log::warning.log("Lua error: %s") << error_string << lj::Log::end;
            b.set_child("error", lj::bson_new_string(error_string));
            b.set_child("is_ok", lj::bson_new_boolean(false));
        }
        else
        {
            b.set_child("is_ok", lj::bson_new_boolean(true));
        }
        lua_pop(L, 1);
        timer.stop();
        
        b.set_child("time/elapsed_usecs", lj::bson_new_uint64(timer.elapsed()));
        
        char* buffer = b.to_binary();
        add_bytes(buffer, b.size());
        delete[] buffer;
        set_writing(true);
    }
}; // namespace logjamd
