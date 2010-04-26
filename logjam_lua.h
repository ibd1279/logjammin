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
#include "BSONNode.h"

namespace logjam {
    void register_logjam_functions(lua_State *L);
    int storage_config_new(lua_State *L);
    int storage_config_save(lua_State *L);
    int storage_config_load(lua_State *L);
    int storage_config_add_index(lua_State *L);
    int storage_config_add_unique(lua_State *L);
    
    class LuaBSONNode {
    private:
        lj::BSONNode *_node;
        bool _gc;
    public:
        LuaBSONNode(lj::BSONNode *ptr, bool gc);
        
        ~LuaBSONNode();
        
        //! Lua bindings class name.
        static const char LUNAR_CLASS_NAME[];
        
        //! Lua bindings method array.
        static Lunar<LuaBSONNode>::RegType LUNAR_METHODS[];
        
        //! Create a new document node for lua.
        LuaBSONNode(lua_State *L);
        
        int nav(lua_State *L);
        
        int set(lua_State *L);
        
        int get(lua_State *L);
        
        int save(lua_State *L);
        
        int load(lua_State *L);
        
        inline lj::BSONNode &node() { return *_node; }
    };
    
};