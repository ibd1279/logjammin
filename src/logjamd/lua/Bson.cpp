/*!
 \file logjamd/lua/Bson.cpp
 \brief Logjamd lj::Bson wrapper implementation
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

#include "Bson.h"
#include <iostream>

namespace logjamd
{
    namespace lua
    {
        const char Bson::LUNAR_CLASS_NAME[] = "Bson";
        Lunar<Bson>::RegType Bson::LUNAR_METHODS[] = {
        LUNAR_MEMBER_METHOD(Bson, nav),
        LUNAR_MEMBER_METHOD(Bson, set),
        LUNAR_MEMBER_METHOD(Bson, push),
        LUNAR_MEMBER_METHOD(Bson, get),
        LUNAR_MEMBER_METHOD(Bson, load),
        LUNAR_MEMBER_METHOD(Bson, save),
        LUNAR_MEMBER_METHOD(Bson, __tostring),
        LUNAR_MEMBER_METHOD(Bson, __index),
        {0, 0, 0}
        };
        
        Bson::Bson(lj::Bson *ptr, bool gc) : root_(ptr), node_(ptr), gc_(gc), path_("")
        {
        }
        
        Bson::~Bson()
        {
            root_ = NULL;
            if (gc_ && node_)
            {
                delete node_;
            }
        }
        
        Bson::Bson(lua_State *L) : root_(NULL), node_(NULL), gc_(true), path_("")
        {
            if (lua_gettop(L) > 0)
            {
                Bson* ptr = Lunar<Bson>::check(L, -1);
                node_ = new lj::Bson(*ptr->node_);
                root_ = node_;
            }
            else
            {
                node_ = new lj::Bson();
                root_ = node_;
            }
        }
        
        Bson::Bson(lj::Bson* root, const std::string& path, bool gc) : root_(root), node_(NULL), gc_(gc), path_(path)
        {
            node_ = root->path(path_);
        }

        
        int Bson::nav(lua_State *L)
        {
            std::string path(lua_to_string(L, -1));
            // XXX This could be a possible source of memory coruption if 
            // XXX the root was GC'd but the user tried to continue using the
            // XXX the returned node.
            Bson* ptr = new Bson(root_, path_ + "/" + path, false);
            Lunar<Bson>::push(L, ptr, true);
            return 1;
        }
        
        int Bson::set(lua_State *L)
        {
            int h = 0;
            lj::Bson* ptr;
            record_delta();
            switch (lua_type(L, -1))
            {
                case LUA_TSTRING:
                    ptr = lj::bson_new_string(lua_to_string(L, -1));
                    real_node().copy_from(*ptr);
                    delete ptr;
                    break;
                case LUA_TNUMBER:
                    ptr = lj::bson_new_int64(luaL_checkint(L, -1));
                    real_node().copy_from(*ptr);
                    delete ptr;
                    break;
                case LUA_TNIL:
                    real_node().nullify();
                    break;
                case LUA_TBOOLEAN:
                    ptr = lj::bson_new_boolean(lua_toboolean(L, -1));
                    real_node().copy_from(*ptr);
                    delete ptr;
                    break;
                case LUA_TUSERDATA:
                case LUA_TLIGHTUSERDATA:
                    ptr = &Lunar<Bson>::check(L, -1)->real_node();
                    real_node().copy_from(*ptr);
                    break;
                case LUA_TTABLE:
                    h = lua_objlen(L, -1);
                    for (int i = 1; i <= h; ++i)
                    {
                        lua_rawgeti(L, -1, i);
                        ptr = new lj::Bson(Lunar<Bson>::check(L, -1)->real_node());
                        real_node().push_child("", ptr);
                        lua_pop(L, 1);
                    }
                    break;
                case LUA_TFUNCTION:
                case LUA_TTHREAD:
                case LUA_TNONE:
                default:
                    break;
            }
            return 0;
        }
        
        int Bson::push(lua_State *L)
        {
            int h = 0;
            lj::Bson* ptr;
            record_delta();
            switch (lua_type(L, -1))
            {
                case LUA_TSTRING:
                    ptr = lj::bson_new_string(lua_to_string(L, -1));
                    real_node().push_child("", ptr);
                    break;
                case LUA_TNUMBER:
                    ptr = lj::bson_new_int64(luaL_checkint(L, -1));
                    real_node().push_child("", ptr);
                    break;
                case LUA_TNIL:
                    ptr = lj::bson_new_null();
                    real_node().push_child("", ptr);
                    break;
                case LUA_TBOOLEAN:
                    ptr = lj::bson_new_boolean(lua_toboolean(L, -1));
                    real_node().push_child("", ptr);
                    break;
                case LUA_TUSERDATA:
                case LUA_TLIGHTUSERDATA:
                    ptr = new lj::Bson(Lunar<Bson>::check(L, -1)->real_node());
                    real_node().push_child("", ptr);
                    break;
                case LUA_TTABLE:
                    h = lua_objlen(L, -1);
                    for (int i = 1; i <= h; ++i)
                    {
                        lua_rawgeti(L, -1, i);
                        ptr = new lj::Bson(Lunar<Bson>::check(L, -1)->real_node());
                        real_node().push_child("", ptr);
                        lua_pop(L, 1);
                    }
                    break;
                case LUA_TFUNCTION:
                case LUA_TTHREAD:
                case LUA_TNONE:
                default:
                    break;
            }
            return 0;
        }
        
        int Bson::get(lua_State *L)
        {
            switch (real_node().type())
            {
                case lj::Bson::k_int32:
                case lj::Bson::k_int64:
                case lj::Bson::k_timestamp:
                    lua_pushinteger(L, lj::bson_as_int64(real_node()));
                    break;
                case lj::Bson::k_array:
                case lj::Bson::k_document:
                case lj::Bson::k_string:
                    lua_pushstring(L, lj::bson_as_string(real_node()).c_str());
                    break;
                case lj::Bson::k_double:
                    lua_pushnumber(L, lj::bson_as_double(real_node()));
                    break;
                case lj::Bson::k_boolean:
                    lua_pushboolean(L, lj::bson_as_boolean(real_node()));
                    break;
                default:
                    lua_pushnil(L);
                    break;
            }
            return 1;
        }
        
        int Bson::save(lua_State *L)
        {
            std::string fn(lua_to_string(L, -1));
            lj::bson_save(*node_, fn);
            return 0;
        }
        
        int Bson::load(lua_State *L)
        {
            std::string fn(lua_to_string(L, -1));
            if (gc_)
            {
                delete node_;
            }
            node_ = lj::bson_load(fn);
            return 0;
        }
        
        int Bson::__tostring(lua_State *L)
        {
            lua_pushstring(L, bson_as_pretty_string(*node_).c_str());
            return 1;
        }
        
        int Bson::__index(lua_State* L)
        {
            std::string key(lua_to_string(L, -1));
            lua_getglobal(L, LUNAR_CLASS_NAME);
            lua_pushvalue(L, -2);
            lua_gettable(L, -2);
            if (lua_isnil(L, -1))
            {
                lua_pop(L, 2);
                return nav(L);
            }
            else
            {
                lua_insert(L, -3);
                lua_pop(L, 2);
                return 1;
            }
        }
        
        void Bson::record_delta()
        {
            if(!lj::bson_as_boolean(root_->nav("__dirty")))
            {
                root_->set_child("__dirty", lj::bson_new_boolean(true));
                root_->nav("__delta").destroy();
            }
            root_->nav("__delta").set_child(lj::bson_escape_path(path_),
                                            lj::bson_new_null());
        }
    }; // namespace logjamd::lua
}; // namespace logjamd
