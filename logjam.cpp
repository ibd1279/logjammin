/*
 \file logjam.cpp
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


#include "build/default/config.h"
#include <iostream>
#include <list>
#include "lunar.h"
#include "logjam_lua.h"
#include "logjam_net.h"
#include "Logger.h"
#include "Exception.h"
#include "LinkedMap.h"
extern "C" {
#include "lualib.h"
}

#ifdef HAVE_EDITLINE
#include <histedit.h>
#endif

namespace {
    void read_from_cin(bool echo, bool prompt, lua_State *L) {
        while(std::cin.good()) {
            std::string buffer;
            if(prompt) {
                std::cout << ">" << std::flush;
            }
            getline(std::cin, buffer);
            if(echo) {
                std::cout << buffer << std::endl;
            }
            int error = luaL_loadbuffer(L, buffer.c_str(), buffer.size(), "line") || lua_pcall(L, 0, 0, 0);
            if(error) {
                std::cerr << lua_tostring(L, -1) << std::endl;
                lua_pop(L, 1);  /* pop error message from the stack */
            }
        }
    }
    
#ifdef HAVE_EDITLINE
    char *editline_prompt(EditLine *e) {
        return ">";
    }
    void input_loop(lua_State *L) {
        EditLine *el = el_init("logjam", stdin, stdout, stderr);
        History *hist = history_init();
        HistEvent ev;
        
        el_set(el, EL_PROMPT, &editline_prompt);
        el_set(el, EL_EDITOR, "vi");
        history(hist, &ev, H_SETSIZE, 100);
        el_set(el, EL_HIST, history, hist);
        
        while(1) {
            int sz;
            const char *line = el_gets(el, &sz);
            if(!line)
                break;
            if(sz && line && *line) {
                history(hist, &ev, H_ENTER, line);
            } else {
                continue;
            }
            if(std::string("quit\n").compare(line) == 0 || std::string("exit\n").compare(line) == 0)
                break;
            int error = luaL_loadbuffer(L, line, sz, "line") || lua_pcall(L, 0, 0, 0);
            if(error) {
                std::cerr << lua_tostring(L, -1) << std::endl;
                lua_pop(L, 1);  /* pop error message from the stack */
            }
        }
        history_end(hist);
        el_end(el);
    }
#else
    void input_loop(lua_State *L) {
        read_from_cin(false, true, L);
    }
#endif
};



int main(int argc, char * const argv[]) {
    lj::Log::debug.disable();
    lj::Log::info.disable();
    
    lj::Bson b;
    b.set_child("cmd", lj::bson_new_string("print(\"Hello World.\")"));
    b.set_child("foobar", lj::bson_new_boolean(false));
    b.set_child("foobar2/testing", lj::bson_new_null());
    
    logjam::Send_bytes* sb = new logjam::Send_bytes(b.to_binary(), b.size());
    
    lj::Socket_selector sl;
    
    sl.connect("127.0.0.1", 27754, sb);
    sl.select();
    
    return 0;
    
    /*
    lua_State *L = lua_open();
    luaL_openlibs(L);
    logjam::register_logjam_functions(L);
    
    if(argc > 1 && strcmp(argv[1], "-") == 0) {
        read_from_cin(true, false, L);
    } else {
        input_loop(L);
    }
    
    lua_close(L);
    return 0;
     */
}