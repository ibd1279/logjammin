#pragma once
/*
 \file Response.h
 \author Jason Watson
 Copyright (c) 2009, Jason Watson
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
#include <list>
#include <string>
#include "lunar.h"

namespace CGI {
    class Request;
    
    class Response {
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<Response>::RegType LUNAR_METHODS[];
        
        static std::string percent_encode(const std::string &input, bool spaces_as_plus=true);
        
        typedef std::multimap<std::string, std::string> header_map;
        
        Response();
        Response(lua_State *L);
        ~Response();
        
        bool is_closed() { return _is_closed; };
        
        void redirect(const std::string &, Request * = NULL);
        void header(const std::string &, const std::string &, const bool replace = true);
        void content_type(const std::string &v) { header("Content-Type", v); };
        void status(int);
        void cookie(const std::string &name, const std::string &value, const std::string &path = "/", long long max_age = -1, bool discard = false);
                
        void write(const std::string &);
        int write(lua_State *);
        void stream(const std::string &, Request *);
        void execute(const std::string &, Request *);
        int execute(lua_State *);
        void close();
    private:
        bool _is_closed;
        std::string _buffer;
        header_map _headers;
    };
    
};