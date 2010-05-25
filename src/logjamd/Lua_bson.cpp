/*
 *  Lua_bson.cpp
 *  logjammin
 *
 *  Created by Jason Watson on 5/24/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Lua_bson.h"

namespace logjamd
{
    const char Lua_bson::LUNAR_CLASS_NAME[] = "Bson";
    Lunar<Lua_bson>::RegType Lua_bson::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Lua_bson, nav),
    LUNAR_MEMBER_METHOD(Lua_bson, set),
    LUNAR_MEMBER_METHOD(Lua_bson, push),
    LUNAR_MEMBER_METHOD(Lua_bson, get),
    LUNAR_MEMBER_METHOD(Lua_bson, load),
    LUNAR_MEMBER_METHOD(Lua_bson, save),
    LUNAR_MEMBER_METHOD(Lua_bson, __tostring),
    LUNAR_MEMBER_METHOD(Lua_bson, __index),
    {0, 0, 0}
    };
    
    Lua_bson::Lua_bson(lj::Bson *ptr, bool gc) : node_(ptr), gc_(gc)
    {
    }
    
    Lua_bson::~Lua_bson()
    {
        if (gc_ && node_)
        {
            delete node_;
        }
    }
    
    Lua_bson::Lua_bson(lua_State *L) : node_(NULL), gc_(true)
    {
        if (lua_gettop(L) > 0)
        {
            Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
            node_ = new lj::Bson(*ptr->node_);
        }
        else
        {
            node_ = new lj::Bson();
        }
    }
    
    int Lua_bson::nav(lua_State *L)
    {
        std::string path(lua_to_string(L, -1));
        // XXX This could be a possible source of memory coruption if 
        // XXX the root was GC'd but the user tried to continue using the
        // XXX the returned node.
        Lunar<Lua_bson>::push(L, new Lua_bson(&node_->nav(path), false), true);
        return 1;
    }
    
    int Lua_bson::set(lua_State *L)
    {
        int h = 0;
        lj::Bson* ptr;
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
                ptr = &Lunar<Lua_bson>::check(L, -1)->real_node();
                real_node().copy_from(*ptr);
                break;
            case LUA_TTABLE:
                h = lua_objlen(L, -1);
                for (int i = 1; i <= h; ++i)
                {
                    lua_rawgeti(L, -1, i);
                    ptr = new lj::Bson(Lunar<Lua_bson>::check(L, -1)->real_node());
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
    
    int Lua_bson::push(lua_State *L)
    {
        int h = 0;
        lj::Bson* ptr;
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
                ptr = new lj::Bson(Lunar<Lua_bson>::check(L, -1)->real_node());
                real_node().push_child("", ptr);
                break;
            case LUA_TTABLE:
                h = lua_objlen(L, -1);
                for (int i = 1; i <= h; ++i)
                {
                    lua_rawgeti(L, -1, i);
                    ptr = new lj::Bson(Lunar<Lua_bson>::check(L, -1)->real_node());
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
    
    int Lua_bson::get(lua_State *L)
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
    
    int Lua_bson::save(lua_State *L)
    {
        std::string fn(lua_to_string(L, -1));
        lj::bson_save(*node_, fn);
        return 0;
    }
    
    int Lua_bson::load(lua_State *L)
    {
        std::string fn(lua_to_string(L, -1));
        if (gc_)
        {
            delete node_;
        }
        node_ = lj::bson_load(fn);
        return 0;
    }
    
    int Lua_bson::__tostring(lua_State *L)
    {
        lua_pushstring(L, bson_as_pretty_string(*node_).c_str());
        return 1;
    }
    
    int Lua_bson::__index(lua_State* L)
    {
        return nav(L);
    }
    
}; // namespace logjamd