/*
 \file main.cpp
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
#include "Storage.h"
#include "LuaStorageFactory.h"
extern "C" {
#include "lualib.h"
}

#ifdef HAVE_EDITLINE
#include <histedit.h>
#endif

namespace {
    void read_from_cin(bool prompt, lua_State *L) {
        while(std::cin.good()) {
            std::string buffer;
            if(prompt) {
                std::cout << ">" << std::flush;
            }
            getline(std::cin, buffer);
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
        read_from_cin(true, L);
    }
#endif
};


int main(int argc, char * const argv[]) {
    lua_State *L = lua_open();
    luaL_openlibs(L);
    Lunar<lj::BSONNode>::Register(L);
    Lunar<logjam::LuaStorageFactory>::Register(L);
    Lunar<logjam::LuaStorageFactory>::push(L, new logjam::LuaStorageFactory(), true);
    lua_setglobal(L, "StorageFactory");

    lj::Storage s("role");
    if(argc > 1 && strcmp(argv[1], "-") == 0) {
        read_from_cin(false, L);
    } else {
        input_loop(L);
    }
    
    lua_close(L);
    return 0;
}

/*
#include "Request.h"
#include "Response.h"
#include "Controller.h"
#include "ProjectController.h"
#include "ReleaseController.h"
#include "BacklogController.h"
#include "UserController.h"
#include "Seed.h"
#include "RssController.h"
#include "RoleController.h"

int main (int argc, char * const argv[]) {
        
    // Create the request/response wrappers.
    CGI::Request *request = new CGI::Request();
    CGI::Response *response = new CGI::Response();
    
    request->context_object("request", request, false);
    request->context_object("response", response, false);
    
    // Create an array of the controllers in the order to evaluate.
    logjammin::controller::Controller *controllers[] = {
        new logjammin::controller::AuthenticateFilter(),
        new logjammin::controller::ImpersonationFilter(),
        new logjammin::controller::HttpHeadersFilter(),
        new logjammin::controller::MessageExpanderFilter(),
        new logjammin::controller::TemplateTopFilter(),
        new logjammin::controller::Seed(),
        new logjammin::controller::StaticAssetController(),
        new logjammin::controller::BacklogEditController(),
        new logjammin::controller::BacklogPurgeController(),
        new logjammin::controller::BacklogListController(),
        new logjammin::controller::ReleaseListController(),
        new logjammin::controller::ProjectEditController(),
        new logjammin::controller::ProjectPurgeController(),
        new logjammin::controller::ProjectListController(),
        new logjammin::controller::UserEditController(),
        new logjammin::controller::UserSearchController(),
        new logjammin::controller::UserPurgeController(),
        new logjammin::controller::UserListController(),
        new logjammin::controller::RoleEditController(),
        new logjammin::controller::RolePurgeController(),
        new logjammin::controller::RoleListController(),
        new logjammin::controller::CommitFeedController(),
        new logjammin::controller::NotFoundController(),
        new logjammin::controller::TemplateBottomFilter(),
        0
    };

    try {
        for(logjammin::controller::Controller **iter = controllers; *iter; ++iter) {
            if((*iter)->is_requested(request, response))
                (*iter)->execute(request, response);
            if(response->is_closed())
                break;
        }
    } catch(std::string &ex) {
        std::cerr << ex << std::endl;
        response->status(500);
    } catch(tokyo::Exception &ex) {
        std::cerr << ex.msg << std::endl;
        response->status(500);
    }
    
    response->close();
    delete response;
    return 0;
}*/