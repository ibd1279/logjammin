/*
 \file User.cpp
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

extern "C" {
#include <openssl/evp.h>
}
#include "User.h"

using tokyo::StorageFilter;

namespace logjammin {
    //=====================================================================
    // User Database
    //=====================================================================
    namespace {
        tokyo::Storage *user_storage() {
            static tokyo::Storage dbo("/var/db/logjammin/user");
            return &dbo;
        }
    }; // namespace
        
    //=====================================================================
    // User Lua Integration
    //=====================================================================
    namespace {
        int User_projects(User *obj, lua_State *L) {
            return 0;
        }
        
        int User_allowed(User *obj, lua_State *L) {
            lua_newtable(L);
            int i = 0;
            std::set<std::string> actions = obj->field("/allowed").to_str_set();
            for(std::set<std::string>::const_iterator iter = actions.begin();
                iter != actions.end();
                ++iter) {
                lua_pushstring(L, iter->c_str());
                lua_rawseti(L, -2, ++i);
            }
            return 1;
        }
        
        int User_denied(User *obj, lua_State *L) {
            lua_newtable(L);
            int i = 0;
            std::set<std::string> actions = obj->field("/denied").to_str_set();
            for(std::set<std::string>::const_iterator iter = actions.begin();
                iter != actions.end();
                ++iter) {
                lua_pushstring(L, iter->c_str());
                lua_rawseti(L, -2, ++i);
            }
            return 1;
        }
        
        int User_logins(User *obj, lua_State *L) {
            lua_newtable(L);
            int i = 0;
            std::set<std::string> logins = obj->field("/logins").to_str_set();
            for(std::set<std::string>::const_iterator iter = logins.begin();
                iter != logins.end();
                ++iter) {
                lua_pushstring(L, iter->c_str());
                lua_rawseti(L, -2, ++i);
            }
            return 1;
        }
        
        int User_check_allowed(User *obj, lua_State *L) {
            const char *action = luaL_checkstring(L, -1);
            lua_pushboolean(L, obj->check_allowed(std::string(action)));
            return 1;
        }
        
        int User_role(User *obj, lua_State *L) {
            Role *ptr = new Role(obj->role());
            Lunar<Role>::push(L, ptr, true);
            return 1;
        }
        
        int User_name(User *obj, lua_State *L) {
            lua_pushstring(L, obj->field("/name").to_str().c_str());
            return 1;
        }
        
        int User_email(User *obj, lua_State *L) {
            lua_pushstring(L, obj->field("/email").to_str().c_str());
            return 1;
        }
        
        int User_aim(User *obj, lua_State *L) {
            lua_pushstring(L, obj->field("/aim").to_str().c_str());
            return 1;
        }
        
        int User_login_count(User *obj, lua_State *L) {
            lua_pushinteger(L, obj->field("login_count").to_int());
            return 1;
        }
        
    }; // namespace
    
    const char User::LUNAR_CLASS_NAME[] = "User";
    
    Lunar<User>::RegType User::LUNAR_METHODS[] = {
    {0,0}
    };
    
    //=====================================================================
    // User Static Methods
    //=====================================================================
    
    void User::all(std::list<User *> &results) {
        user_storage()->all().items<User>(results);
    }
    
    void User::like(const std::string &term,
                    std::list<User *> &results) {
        StorageFilter sf = user_storage()->none().mode(StorageFilter::UNION);
        sf.search("/email", term).search("/name", term).search("/aim", term);
        sf.items<User>(results);
    }
    
    void User::at(unsigned long long key,
                  User &model) {
        tokyo::Document d(user_storage()->at(key));
        model._d.swap(d);
    }
    
    void User::at_login(const std::string &login, User &model) {
        tokyo::StorageFilter sf = user_storage()->filter("/login",
                                                         login.c_str(),
                                                         login.size() + 1);
        
        if(sf.size() == 0) {
            throw std::string("Unknown User Login ").append(login).append(".");
        } else if(sf.size() > 1) {
            throw std::string("Ambiguous User Login ").append(login).append(".");
        }
        
        std::list<User *> results;
        sf.items<User>(results);
        model._d.swap(results.front()->_d);
        for(std::list<User *>::const_iterator iter = results.begin();
            iter != results.end();
            ++iter) {
            delete *iter;
        }
    }
    
    //=====================================================================
    // User ctor/dtor
    //=====================================================================
    
    User::~User() {
        if(_cached_allowed)
            delete _cached_allowed;
        _cached_allowed = NULL;
    }
    
    //=====================================================================
    // User Instance
    //=====================================================================
    
    void User::role(const Role &role) {
        if(_cached_allowed)
            delete _cached_allowed;
        _cached_allowed = NULL;
        field("/role", (long long)role.pkey());
    }
    
    namespace {
        std::string digest_string(const std::string &msg) {
            EVP_MD_CTX mdctx;
            EVP_MD_CTX_init(&mdctx);
            
            const EVP_MD *md = EVP_sha1();
            EVP_DigestInit_ex(&mdctx, md, NULL);
            EVP_DigestUpdate(&mdctx, msg.c_str(), msg.size());
            
            unsigned char *md_value = new unsigned char[EVP_MAX_MD_SIZE];
            unsigned int md_len;
            EVP_DigestFinal(&mdctx, md_value, &md_len);
            EVP_MD_CTX_cleanup(&mdctx);
            
            std::ostringstream result;
            for(int h = 0; h < md_len; ++h) {
                char buffer[3];
                sprintf(buffer, "%02x", md_value[h]);
                result << buffer;
            }
            
            free(md_value);
            return result.str();
        }
    };
    
    void User::cookie(const std::string &cookie) {
        field("/cookie", digest_string(cookie));
    }
    
    bool User::check_cookie(const std::string &cookie) {
        return (field("/cookie").to_str().compare(digest_string(cookie)) == 0);
    }
    
    bool User::check_allowed(const std::string &action) {
        if(!_cached_allowed) {
            _cached_allowed = new std::set<std::string>();
            
            std::set<std::string> actions = role().field("/allowed").to_str_set();
            _cached_allowed->insert(actions.begin(), actions.end());
            
            actions = field("/allowed").to_str_set();
            _cached_allowed->insert(actions.begin(), actions.end());
            
            actions = field("/denied").to_str_set();
            for(std::set<std::string>::const_iterator iter = actions.begin();
                iter != actions.end();
                ++iter)
                _cached_allowed->erase(*iter);
        }
        
        return (_cached_allowed->count(action) == 1);
    }
    
    tokyo::Storage *User::dao() const {
        return user_storage();
    }
}; // namespace logjammin    
