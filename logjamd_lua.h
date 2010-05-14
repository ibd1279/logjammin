#pragma once
/*
 \file logjam_lua.h
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

#include "lunar.h"
#include "Bson.h"
#include "Storage.h"

namespace logjamd {
    void register_logjam_functions(lua_State *L);
    int storage_config_new(lua_State *L);
    int storage_config_save(lua_State *L);
    int storage_config_load(lua_State *L);
    int storage_config_add_index(lua_State *L);
    int storage_config_add_nested_field(lua_State *L);
    
    //! Lua Bson wrapper.
    /*!
     \par
     Known as "Bson" in lua.
     \par
     TODO This should be modified to override the __index method on the meta
     table to get children. get() should probably be replaced with type
     specific getters.
     \author Jason Watson
     \version 1.0
     \date April 26, 2010
     */
    class LuaBSONNode {
    private:
        lj::Bson *_node;
        bool _gc;
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<LuaBSONNode>::RegType LUNAR_METHODS[];
        LuaBSONNode(lua_State *L);
        LuaBSONNode(lj::Bson *ptr, bool gc);
        ~LuaBSONNode();
        int nav(lua_State *L);
        int set(lua_State *L);
        int push(lua_State *L);
        int get(lua_State *L);
        int save(lua_State *L);
        int load(lua_State *L);
        int __tostring(lua_State *L);
        inline lj::Bson &real_node() { return *_node; }
    };
    
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

    //! Lua Storage wrapper.
    /*!
     \par
     Known as "Storage" in lua.
     \author Jason Watson
     \version 1.0
     \date April 27, 2010
     */
    class LuaStorage {
    private:
        lj::Storage *_storage;
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<LuaStorage>::RegType LUNAR_METHODS[];
        LuaStorage(lua_State *L);
        ~LuaStorage();
        int all(lua_State *L);
        int none(lua_State *L);
        int at(lua_State *L);
        int place(lua_State *L);
        int remove(lua_State *L);
        inline lj::Storage &real_storage() { return *_storage; }
    };        
};