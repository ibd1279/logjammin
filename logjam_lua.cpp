/*
 \file logjam_lua.cpp
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

#include "build/default/config.h"
#include <string>
#include <sstream>
#include "logjam_lua.h"
#include "Logger.h"

// This should be moved somewhere for portability.
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>


using lj::Log;

namespace logjam {
    void register_logjam_functions(lua_State *L) {
        Lunar<logjam::LuaBSONNode>::Register(L);
        Lunar<logjam::LuaStorageFilter>::Register(L);
        Lunar<logjam::LuaStorage>::Register(L);
        lua_pushcfunction(L, &storage_config_new);
        lua_setglobal(L, "sc_new");
        lua_pushcfunction(L, &storage_config_save);
        lua_setglobal(L, "sc_save");
        lua_pushcfunction(L, &storage_config_load);
        lua_setglobal(L, "sc_load");
        lua_pushcfunction(L, &storage_config_add_index);
        lua_setglobal(L, "sc_add_index");
        lua_pushcfunction(L, &storage_config_add_unique);
        lua_setglobal(L, "sc_add_unique");
    }
    
    //=====================================================================
    // logjam global functions.
    //=====================================================================
    std::string lua_to_string(lua_State *L, int offset) {
        const char *ptr = luaL_checkstring(L, offset);
        if(!ptr)
            return std::string();
        size_t l = lua_strlen(L, offset);
        return std::string(ptr, l);
    }
    
    int storage_config_new(lua_State *L) {
        std::string dbname(lua_to_string(L, -1));
        lj::Bson *ptr = new lj::Bson();
        ptr->nav("main/compare").set_string(std::string("int64"));
        ptr->nav("main/file").set_string(std::string("db_") + dbname + ".tcb");
        ptr->nav("main/mode/0").set_string("create");
        ptr->nav("main/mode/1").set_string("read");
        ptr->nav("main/mode/2").set_string("write");
        ptr->nav("main/type").set_string("tree");
        ptr->nav("main/unique");
        ptr->nav("index/tree");
        ptr->nav("index/text");
        ptr->nav("index/tag");
        ptr->nav("index/hash");
        Lunar<LuaBSONNode>::push(L, new LuaBSONNode(ptr, true), true);
        return 1;
    }
    
    int storage_config_save(lua_State *L) {
        std::string dbname(lua_to_string(L, -2));
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -1);
        std::string dbfile(DBDIR);
        if(dbname.size() > 1 && dbname[dbname.size() - 1] == '/')
            dbfile.append(dbname);
        else
            dbfile.append("/").append(dbname);
        
        // This should be moved somewhere for portability.
        if(mkdir(dbfile.c_str(), S_IRWXU | S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP)) {
            return luaL_error(L, "Failed to create directory [%d][%s].", errno, strerror(errno));
        }
        
        dbfile.append("/config");
        ptr->real_node().save(dbfile);
        return 0;
    }
    
    int storage_config_load(lua_State *L) {
        std::string dbname(lua_to_string(L, -1));
        std::string dbfile(DBDIR);
        if(dbname.size() > 1 && dbname[dbname.size() - 1] == '/')
            dbfile.append(dbname);
        else
            dbfile.append("/").append(dbname);
        dbfile.append("/config");
        lj::Bson *ptr = new lj::Bson();
        ptr->load(dbfile);
        Lunar<LuaBSONNode>::push(L, new LuaBSONNode(ptr, true), true);
        return 1;
    }
    
    int storage_config_add_index(lua_State *L) {
        std::string indxcomp(lua_to_string(L, -1));
        std::string indxfield(lua_to_string(L, -2));
        std::string indxname(lua_to_string(L, -3));
        std::string indxtype(lua_to_string(L, -4));
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -5);
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/compare").set_string(indxcomp);
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/file").set_string(std::string("index.") + indxname + "." + indxtype + ".tc");
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/mode/0").set_string("create");
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/mode/1").set_string("read");
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/mode/2").set_string("write");
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/type").set_string(indxtype);
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/field").set_string(indxfield);
        ptr->real_node().nav(std::string("index/") + indxtype + "/" + indxname + "/children").set_boolean(false);
        return 0;
    }
    
    int storage_config_add_unique(lua_State *L) {
        std::string field(lua_to_string(L, -1));
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -2);
        
        std::set<std::string> allowed(ptr->real_node().nav("main/unique").to_set());
        allowed.insert(field);
        
        lj::Bson n;
        int h = 0;
        for(std::set<std::string>::const_iterator iter = allowed.begin();
            iter != allowed.end();
            ++iter) {
            std::ostringstream buf;
            buf << h++;
            lj::Bson tmp;
            tmp.set_string(*iter);
            n.child(buf.str(), tmp);
        }
        ptr->real_node().nav("main/unique").assign(n);
        return 0;
    }
    
    //=====================================================================
    // Bson Lua integration Fields
    //=====================================================================
    const char LuaBSONNode::LUNAR_CLASS_NAME[] = "Bson";
    Lunar<LuaBSONNode>::RegType LuaBSONNode::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(LuaBSONNode, nav),
    LUNAR_MEMBER_METHOD(LuaBSONNode, set),
    LUNAR_MEMBER_METHOD(LuaBSONNode, get),
    LUNAR_MEMBER_METHOD(LuaBSONNode, load),
    LUNAR_MEMBER_METHOD(LuaBSONNode, save),
    LUNAR_MEMBER_METHOD(LuaBSONNode, __tostring),
    {0, 0}
    };

    //=====================================================================
    // Bson Lua integration Methods.
    //=====================================================================
    LuaBSONNode::LuaBSONNode(lj::Bson *ptr, bool gc) : _node(ptr), _gc(gc) {
    }
    
    LuaBSONNode::~LuaBSONNode() {
        if(_gc && _node)
            delete _node;
    }
    
    LuaBSONNode::LuaBSONNode(lua_State *L) : _node(NULL), _gc(true) {
        if(lua_gettop(L) > 0) {
            LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -1);
            _node = new lj::Bson(*ptr->_node);
        } else {
            _node = new lj::Bson();
        }
    }

    int LuaBSONNode::nav(lua_State *L) {
        std::string path(lua_to_string(L, -1));
        // XXX This could be a possible source of memory coruption if 
        // XXX the root was GC'd but the user tried to continue using the
        // XXX the returned node.
        Lunar<LuaBSONNode>::push(L, new LuaBSONNode(&_node->nav(path), false), true);
        return 1;
    }
    
    int LuaBSONNode::set(lua_State *L) {
        switch(lua_type(L, -1)) {
            case LUA_TSTRING:
                _node->set_string(lua_to_string(L, -1));
                break;
            case LUA_TNUMBER:
                _node->set_int32(luaL_checkint(L, -1));
                break;
            case LUA_TNIL:
                _node->nullify();
                break;
            case LUA_TBOOLEAN:
                _node->set_boolean(static_cast<bool>(lua_toboolean(L, -1)));
                break;
            case LUA_TTABLE:
            case LUA_TFUNCTION:
            case LUA_TTHREAD:
            case LUA_TUSERDATA:
            case LUA_TLIGHTUSERDATA:
            case LUA_TNONE:
            default:
                break;
        }
        return 0;
    }
    
    int LuaBSONNode::get(lua_State *L) {
        switch(_node->type()) {
            case lj::k_bson_int32:
            case lj::k_bson_int64:
            case lj::k_bson_timestamp:
                lua_pushinteger(L, _node->to_l());
                break;
            case lj::k_bson_document:
            case lj::k_bson_array:
            case lj::k_bson_string:
                lua_pushstring(L, _node->to_s().c_str());
                break;
            case lj::k_bson_double:
                lua_pushnumber(L, _node->to_d());
                break;
            case lj::k_bson_boolean:
                lua_pushboolean(L, _node->to_b());
                break;
            default:
                lua_pushnil(L);
                break;
        }
        return 1;
    }
    
    int LuaBSONNode::save(lua_State *L) {
        std::string fn(lua_to_string(L, -1));
        _node->save(fn);
        return 0;
    }
    
    int LuaBSONNode::load(lua_State *L) {
        std::string fn(lua_to_string(L, -1));
        _node->load(fn);
        return 0;
    }
    
    int LuaBSONNode::__tostring(lua_State *L) {
        lua_pushstring(L, _node->to_s().c_str());
        return 1;
    }
    
    //=====================================================================
    // Storage Filter Lua integration Methods.
    //=====================================================================
    
    const char LuaStorageFilter::LUNAR_CLASS_NAME[] = "Record_set";
    Lunar<LuaStorageFilter>::RegType LuaStorageFilter::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(LuaStorageFilter, mode_and),
    LUNAR_MEMBER_METHOD(LuaStorageFilter, mode_or),
    LUNAR_MEMBER_METHOD(LuaStorageFilter, filter),
    LUNAR_MEMBER_METHOD(LuaStorageFilter, search),
    LUNAR_MEMBER_METHOD(LuaStorageFilter, tagged),
    LUNAR_MEMBER_METHOD(LuaStorageFilter, records),
    LUNAR_MEMBER_METHOD(LuaStorageFilter, first),
    {0, 0}
    };
    
    LuaStorageFilter::LuaStorageFilter(lua_State *L) : _filter(NULL) {
        LuaStorage *ptr = Lunar<LuaStorage>::check(L, -1);
        _filter = new lj::Record_set(ptr->real_storage().none());
    }
    
    LuaStorageFilter::LuaStorageFilter(lj::Record_set *filter) : _filter(filter) {
    }
    
    LuaStorageFilter::~LuaStorageFilter() {
        if(_filter)
            delete _filter;
    }
    
    int LuaStorageFilter::mode_and(lua_State *L) {
        _filter->set_operation(lj::set::k_intersection);
        Lunar<LuaStorageFilter>::push(L, this, false);
        return 1;
    }
    
    int LuaStorageFilter::mode_or(lua_State *L) {
        _filter->set_operation(lj::set::k_union);
        Lunar<LuaStorageFilter>::push(L, this, false);
        return 1;
    }
    
    int LuaStorageFilter::filter(lua_State *L) {
        std::string field(lua_to_string(L, -2));
        if(lua_isstring(L, -1)) {
            std::string val(lua_to_string(L, -1));
            lj::Record_set *ptr = new lj::Record_set(_filter->equal(field, val.c_str(), val.size()));
            Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        } else {
            LuaBSONNode *n = Lunar<LuaBSONNode>::check(L, -1);
            char *bson = n->real_node().bson();
            lj::Record_set *ptr = NULL;
            if(lj::bson_type_is_quotable(n->real_node().type())) {
                ptr = new lj::Record_set(_filter->equal(field, bson + 4, n->real_node().size() - 5));
            } else {
                ptr = new lj::Record_set(_filter->equal(field, bson, n->real_node().size()));
            }
            delete[] bson;
            Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        }
        return 1;
    }

    int LuaStorageFilter::search(lua_State *L) {
        std::string field(lua_to_string(L, -2));
        std::string val(lua_to_string(L, -1));
        lj::Record_set *ptr = new lj::Record_set(_filter->contains(field, val));
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int LuaStorageFilter::tagged(lua_State *L) {
        std::string field(lua_to_string(L, -2));
        std::string val(lua_to_string(L, -1));
        lj::Record_set *ptr = new lj::Record_set(_filter->tagged(field, val));
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int LuaStorageFilter::records(lua_State *L) {
        std::list<lj::Bson *> d;
        int h = 0;
        _filter->items<lj::Bson>(d);
        lua_newtable(L);
        for(std::list<lj::Bson *>::const_iterator iter = d.begin();
            iter != d.end();
            ++iter) {
            Lunar<LuaBSONNode>::push(L, new LuaBSONNode(*iter, true), true);
            lua_rawseti(L, -2, ++h);
        }
        return 1;
    }
    
    int LuaStorageFilter::first(lua_State *L) {
        if(_filter->size() < 1) {
            lua_pushnil(L);
            return 1;
        }
        lj::Bson *d = new lj::Bson();
        _filter->first(*d);
        Lunar<LuaBSONNode>::push(L, new LuaBSONNode(d, true), true);
        return 1;
    }
    
    //=====================================================================
    // Storage Lua integration Methods.
    //=====================================================================
    const char LuaStorage::LUNAR_CLASS_NAME[] = "Storage";
    Lunar<LuaStorage>::RegType LuaStorage::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(LuaStorage, all),
    LUNAR_MEMBER_METHOD(LuaStorage, none),
    LUNAR_MEMBER_METHOD(LuaStorage, at),
    LUNAR_MEMBER_METHOD(LuaStorage, place),
    LUNAR_MEMBER_METHOD(LuaStorage, remove),
    {0, 0}
    };
    
    LuaStorage::LuaStorage(lua_State *L) : _storage(NULL) {
        std::string dbname(lua_to_string(L, -1));
        _storage = new lj::Storage(dbname);
    }
    
    LuaStorage::~LuaStorage() {
        if(_storage)
            delete _storage;
    }
    
    int LuaStorage::all(lua_State *L) {
        lj::Record_set *ptr = new lj::Record_set(_storage->all());
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int LuaStorage::none(lua_State *L) {
        lj::Record_set *ptr = new lj::Record_set(_storage->none());
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int LuaStorage::at(lua_State* L)
    {
        lj::Record_set* ptr = new lj::Record_set(_storage->at(luaL_checkint(L, -1)));
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
        
    int LuaStorage::place(lua_State *L) {
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -1);
        try {
            _storage->place(ptr->real_node());
        } catch(lj::Exception* ex) {
            luaL_error(L, "Unable to place content. %s", ex->to_string().c_str());
        }
        Lunar<LuaStorage>::push(L, this, false);
        return 1;
    }
    
    int LuaStorage::remove(lua_State *L) {
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -1);
        _storage->remove(ptr->real_node());
        Lunar<LuaStorage>::push(L, this, false);
        return 1;
    }
    
};