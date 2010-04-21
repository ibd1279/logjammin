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
#include <iostream>

namespace logjammin {
    //=====================================================================
    // Role Database
    //=====================================================================
    namespace {
        tokyo::Storage * role_storage() {
            static tokyo::Storage dbo("/var/db/logjammin/role");
            return &dbo;
        }
    }; // namespace

    //=====================================================================
    // Role Lua Integration
    //=====================================================================        
    namespace {
        int Role_allowed(Role *obj, lua_State *L) {
            lua_newtable(L);
            int i = 0;
            std::set<std::string> allowed = obj->field("allowed").to_str_set();
            for(std::set<std::string>::const_iterator iter = allowed.begin();
                iter != allowed.end();
                ++iter) {
                lua_pushstring(L, iter->c_str());
                lua_rawseti(L, -2, ++i);
            }
            return 1;
        }
        
        int Role_name(Role *obj, lua_State *L) {
            lua_pushstring(L, obj->field("name").to_str().c_str());
            return 1;
        }
    }; // namespace
    
    const char Role::LUNAR_CLASS_NAME[] = "Role";
    
    Lunar<Role>::RegType Role::LUNAR_METHODS[] = {
    LUNAR_STATIC_METHOD(Role, allowed),
    LUNAR_STATIC_METHOD(Role, name),
    LUNAR_INTEGER_GETTER(Role, pkey, unsigned long long),
    {0,0,0}
    };
    
    //=====================================================================
    // Role Static Methods
    //=====================================================================        
    
    void Role::all(std::list<Role *> &results) {
        role_storage()->all().items<Role>(results);
    }
    
    void Role::at(unsigned long long key,
                  Role &model) {
        tokyo::Document d(role_storage()->at(key));
        model._d.swap(d);
    }
    
    void Role::at_name(const std::string &name, Role &model) {
        tokyo::StorageFilter sf = role_storage()->filter("name",
                                                         name.c_str(),
                                                         name.size() + 1);
        
        if(sf.size() == 0) {
            throw std::string("Unknown Role Name ").append(name).append(".");
        } else if(sf.size() > 1) {
            throw std::string("Ambiguous Role Name ").append(name).append(".");
        }
        
        std::list<Role *> results;
        sf.items<Role>(results);
        model._d.swap(results.front()->_d);
        for(std::list<Role *>::const_iterator iter = results.begin();
            iter != results.end();
            ++iter) {
            delete *iter;
        }
    }
    
    //=====================================================================
    // Role ctor/dtor
    //=====================================================================
        
    Role::~Role() {
    }
    
    //=====================================================================
    // Role Instance
    //=====================================================================
    
    void Role::add_allowed(const std::string &action) {
        std::set<std::string> allowed(_d.path("allowed").to_str_set());
        allowed.insert(action);
        
        tokyo::DocumentNode n(tokyo::DOC_NODE, NULL);
        int h = 0;
        for(std::set<std::string>::const_iterator iter = allowed.begin();
            iter != allowed.end();
            ++iter) {
            std::ostringstream buf;
            buf << h++;
            n.child(buf.str(), tokyo::DocumentNode().value(*iter));
        }
        _d.path("", "allowed", n);
    }
    
    void Role::remove_allowed(const std::string &action) {
        std::set<std::string> allowed(field("allowed").to_str_set());
        allowed.erase(action);

        tokyo::DocumentNode n(tokyo::DOC_NODE, NULL);
        int h = 0;
        for(std::set<std::string>::const_iterator iter = allowed.begin();
            iter != allowed.end();
            ++iter) {
            std::ostringstream buf;
            buf << h++;
            n.child(buf.str(), tokyo::DocumentNode().value(*iter));
        }
        std::cerr << n.to_str() << std::endl;
        _d.path("", "allowed", n);
    }
    
    tokyo::Storage *Role::dao() const {
        return role_storage();
    }
}; // namespace logjammin
