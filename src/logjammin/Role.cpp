/*
 \file Role.cpp
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

#include "Role.h"
#include <sstream>

namespace logjammin {
    //=====================================================================
    // Role Lua Integration
    //=====================================================================
    namespace {
        int Role_allowed(Role* obj, lua_State* L)
        {
            std::set<std::string> allowed(obj->allowed());
            lua_newtable(L);
            for (std::list<lj::Bson *>::const_iterator iter = allowed.begin();
                 iter != allowed.end();
                 ++iter)
            {
                lua_pushstring(L, *iter);
                lua_rawseti(L, -2, ++h);
            }
            return 1;
        }
        
        int Role_add_allowed(Role* obj, lua_State* L)
        {
            std::string action(lua_to_string(L, -1));
            obj->add_allowed(action);
            return 0;
        }
        
        int Role_remove_allowed(Role* obj, lua_State* L)
        {
            std::string action(lua_to_string(L, -1));
            obj->remove_allowed(action);
            return 0;
        }        
    }; // namespace
    
    const char Role::LUNAR_CLASS_NAME[] = "Role";
    
    Lunar<Role>::RegType Role::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Role, __index),
    LUNAR_STATIC_METHOD(Role, allowed),
    LUNAR_STATIC_METHOD(Role, add_allowed),
    LUNAR_STATIC_METHOD(Role, remove_allowed),
    {0,0,0}
    };
    
    Role::Role(lj::Bson* ptr) : doc_(ptr)
    {
    }
    
    Role::~Role()
    {
        if (doc_)
        {
            delete doc_;
        }
    }
    
    int __index(lua_State* L)
    {
        std::string key(lua_to_string(L, -1));
        lua_getglobal(L, LUNAR_CLASS_NAME);
        lua_pushvalue(L, -2);
        lua_gettable(L, -2);
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 2);
            return doc_->nav(L);
        }
        else
        {
            lua_insert(L, -3);
            lua_pop(L, 2);
            return 1;
        }
    }
    
    std::string name()
    {
        return lj::bson_as_string(doc_->nav("name"));
    }
    
    void name(const std::string& v)
    {
        doc_->set_child("name", lj::bson_new_string(v));
    }
    
    std::set<std::string> allowed()
    {
        return lj::bson_as_value_string_set(doc_->nav("allowed"));
    }
    
    void add_allowed(const std::string& action)
    {
        lj::Bson* ptr = doc_->path("allowed");
        std::set<std::string> allowed(lj::bson_as_value_string_set(*ptr));
        
        if (allowed.find(action) == allowed.end())
        {
            ptr->push_child("", lj::bson_new_string(action));
        }
    }
    
    void remove_allowed(const std::string& action)
    {
        lj::Bson* ptr = doc_->path("allowed");
        std::set<std::string> allowed(lj::bson_as_value_string_set(*ptr));
        allowed.erase(action);
        ptr->destroy();
        
        for (std::set<std::string>::const_iterator iter = allowed.begin();
             allowed.end() != iter;
             ++iter)
        {
            ptr->push_child("", lj::bson_new_string(*iter));
        }
    }
    
}; // namespace logjammin