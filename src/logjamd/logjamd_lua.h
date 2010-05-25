#pragma once
/*!
 \file logjamd_lua.h
 \brief Logjam server lua functions implementation.
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

#include "lj/Bson.h"
#include "lj/lunar.h"
#include "lj/Storage.h"

#include <map>

namespace logjamd {
    void register_logjam_functions(lua_State *L);
    
    int connection_config_load(lua_State* L);
    int connection_config_save(lua_State* L);
    int connection_config_add_default_storage(lua_State* L);
    int connection_config_remove_default_storage(lua_State* L);
    
    int send_response(lua_State* L);
    
    //! Lua Record_set wrapper.
    /*!
     \par
     Known as "Record_set" in lua.
     \author Jason Watson
     \version 1.0
     \date April 27, 2010
     */
    class LuaStorageFilter {
    private:
        lj::Record_set *_filter;
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<LuaStorageFilter>::RegType LUNAR_METHODS[];
        LuaStorageFilter(lua_State *L);
        LuaStorageFilter(lj::Record_set *filter);
        ~LuaStorageFilter();
        int mode_and(lua_State *L);
        int mode_or(lua_State *L);
        int filter(lua_State *L);
        int search(lua_State *L);
        int tagged(lua_State *L);
        int records(lua_State *L);
        int first(lua_State *L);
        int size(lua_State *L);
        inline lj::Record_set &real_filter() { return *_filter; }
    };     
};