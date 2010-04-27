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
    
    int storage_config_new(lua_State *L) {
        std::string dbname(luaL_checkstring(L, -1));
        lj::BSONNode *ptr = new lj::BSONNode();
        ptr->nav("main/compare").value(std::string("int64"));
        ptr->nav("main/file").value(std::string("db_") + dbname + ".tcb");
        ptr->nav("main/mode/0").value("create");
        ptr->nav("main/mode/1").value("read");
        ptr->nav("main/mode/2").value("write");
        ptr->nav("main/type").value("tree");
        ptr->nav("main/unique");
        ptr->nav("index/tree");
        ptr->nav("index/text");
        ptr->nav("index/tag");
        ptr->nav("index/hash");
        Lunar<LuaBSONNode>::push(L, new LuaBSONNode(ptr, true), true);
        return 1;
    }
    
    int storage_config_save(lua_State *L) {
        std::string dbname(luaL_checkstring(L, -2));
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -1);
        std::string dbfile(DBDIR);
        if(dbname.size() > 1 && dbname[dbname.size() - 1] == '/')
            dbfile.append(dbname);
        else
            dbfile.append("/").append(dbname);
        
        // This should be moved somewhere for portability.
        if(mkdir(dbfile.c_str(), S_IRWXU | S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP)) {
            throw lj::Exception("logjam", std::string(strerror(errno)));
        }
        
        dbfile.append("/config");
        ptr->node().save(dbfile);
        return 0;
    }
    
    int storage_config_load(lua_State *L) {
        std::string dbname(luaL_checkstring(L, -1));
        std::string dbfile(DBDIR);
        if(dbname.size() > 1 && dbname[dbname.size() - 1] == '/')
            dbfile.append(dbname);
        else
            dbfile.append("/").append(dbname);
        dbfile.append("/config");
        lj::BSONNode *ptr = new lj::BSONNode();
        Log::info("Loading from config file [%s]") << dbfile << Log::end;
        ptr->load(dbfile);
        Lunar<LuaBSONNode>::push(L, new LuaBSONNode(ptr, true), true);
        return 1;
    }
    
    int storage_config_add_index(lua_State *L) {
        std::string indxcomp(luaL_checkstring(L, -1));
        std::string indxfield(luaL_checkstring(L, -2));
        std::string indxname(luaL_checkstring(L, -3));
        std::string indxtype(luaL_checkstring(L, -4));
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -5);
        ptr->node().nav(std::string("index/") + indxtype + "/" + indxname + "/compare").value(indxcomp);
        ptr->node().nav(std::string("index/") + indxtype + "/" + indxname + "/file").value(std::string("index_") + indxname + ".tc_");
        ptr->node().nav(std::string("index/") + indxtype + "/" + indxname + "/mode/0").value("create");
        ptr->node().nav(std::string("index/") + indxtype + "/" + indxname + "/mode/1").value("read");
        ptr->node().nav(std::string("index/") + indxtype + "/" + indxname + "/mode/2").value("write");
        ptr->node().nav(std::string("index/") + indxtype + "/" + indxname + "/type").value(indxtype);
        ptr->node().nav(std::string("index/") + indxtype + "/" + indxname + "/field").value(indxfield);
        return 0;
    }
    
    int storage_config_add_unique(lua_State *L) {
        std::string field(luaL_checkstring(L, -1));
        LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -2);
        
        std::set<std::string> allowed(ptr->node().nav("main/unique").to_set());
        allowed.insert(field);
        
        lj::BSONNode n;
        int h = 0;
        for(std::set<std::string>::const_iterator iter = allowed.begin();
            iter != allowed.end();
            ++iter) {
            std::ostringstream buf;
            buf << h++;
            n.child(buf.str(), lj::BSONNode().value(*iter));
        }
        ptr->node().nav("main/unique").assign(n);
        return 0;
    }
    
    //=====================================================================
    // BSONNode Lua integration
    //=====================================================================
    const char LuaBSONNode::LUNAR_CLASS_NAME[] = "BSONNode";
    
    Lunar<LuaBSONNode>::RegType LuaBSONNode::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(LuaBSONNode, nav),
    LUNAR_MEMBER_METHOD(LuaBSONNode, set),
    LUNAR_MEMBER_METHOD(LuaBSONNode, get),
    LUNAR_MEMBER_METHOD(LuaBSONNode, load),
    LUNAR_MEMBER_METHOD(LuaBSONNode, save),
    {0, 0}
    };

    LuaBSONNode::LuaBSONNode(lj::BSONNode *ptr, bool gc) : _node(ptr), _gc(gc) {
    }
    
    LuaBSONNode::~LuaBSONNode() {
        if(_gc && _node)
            delete _node;
    }
    
    LuaBSONNode::LuaBSONNode(lua_State *L) : _node(NULL), _gc(true) {
        int argc = lua_gettop(L);
        if(argc > 0) {
            LuaBSONNode *ptr = Lunar<LuaBSONNode>::check(L, -1);
            if(ptr)
                _node = new lj::BSONNode(*ptr->_node);
            else
                _node = new lj::BSONNode();
        } else {
            _node = new lj::BSONNode();
        }
    }

    int LuaBSONNode::nav(lua_State *L) {
        std::string path(luaL_checkstring(L, -1));
        // XXX This could be a possible source of memory coruption if 
        // XXX the root was GC'd but the user tried to continue using the
        // XXX the returned node.
        Lunar<LuaBSONNode>::push(L, new LuaBSONNode(&_node->nav(path), false), true);
        return 1;
    }
    
    int LuaBSONNode::set(lua_State *L) {
        const char *str;
        int tmp;
        lua_settop(L, 1);
        switch(lua_type(L, 1)) {
            case LUA_TSTRING:
                str = luaL_checkstring(L, 1);
                _node->value(std::string(str));
                break;
            case LUA_TNUMBER:
                _node->value(luaL_checkint(L, 1));
                break;
            case LUA_TNIL:
                _node->set_value(lj::NULL_NODE, NULL);
                break;
            case LUA_TBOOLEAN:
                tmp = lua_toboolean(L, 1) ? 1 : 0;
                _node->set_value(lj::BOOL_NODE, (char *)&tmp);
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
            case lj::INT32_NODE:
            case lj::INT64_NODE:
            case lj::TIMESTAMP_NODE:
                lua_pushinteger(L, _node->to_l());
                break;
            case lj::DOC_NODE:
            case lj::ARRAY_NODE:
            case lj::STRING_NODE:
                lua_pushstring(L, _node->to_s().c_str());
                break;
            case lj::DOUBLE_NODE:
            default:
                lua_pushnil(L);
                break;
        }
        return 1;
    }
    
    int LuaBSONNode::save(lua_State *L) {
        std::string fn(luaL_checkstring(L, -1));
        _node->save(fn);
        return 0;
    }
    
    int LuaBSONNode::load(lua_State *L) {
        std::string fn(luaL_checkstring(L, -1));
        _node->load(fn);
        return 0;
    }
};