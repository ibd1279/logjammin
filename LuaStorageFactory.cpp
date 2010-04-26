/*
 \file LuaStorageFactory.cpp
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
#include "LuaStorageFactory.h"
#include "BSONNode.h"

namespace logjam {
    const char LuaStorageFactory::LUNAR_CLASS_NAME[] = "LuaStorageFactory";
    Lunar<LuaStorageFactory>::RegType LuaStorageFactory::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(LuaStorageFactory, add_config_index),
    LUNAR_MEMBER_METHOD(LuaStorageFactory, new_config),
    LUNAR_MEMBER_METHOD(LuaStorageFactory, save_config),
    LUNAR_MEMBER_METHOD(LuaStorageFactory, load_config),
    {0, 0}
    };
    std::string LuaStorageFactory::_dbdir(DBDIR);
    
    LuaStorageFactory::LuaStorageFactory() {}
    LuaStorageFactory::LuaStorageFactory(lua_State *L) {}
    LuaStorageFactory::~LuaStorageFactory() {}
    
    int LuaStorageFactory::_add_config_index(lua_State *L) {
        std::string indxcomp(luaL_checkstring(L, -1));
        std::string indxfield(luaL_checkstring(L, -2));
        std::string indxname(luaL_checkstring(L, -3));
        std::string indxtype(luaL_checkstring(L, -4));
        lj::BSONNode *ptr = Lunar<lj::BSONNode>::check(L, -5);
        ptr->nav(std::string("index/") + indxtype + "/" + indxname + "/compare").value(indxcomp);
        ptr->nav(std::string("index/") + indxtype + "/" + indxname + "/file").value(std::string("index_") + indxname + ".tc_");
        ptr->nav(std::string("index/") + indxtype + "/" + indxname + "/mode/0").value("create");
        ptr->nav(std::string("index/") + indxtype + "/" + indxname + "/mode/1").value("read");
        ptr->nav(std::string("index/") + indxtype + "/" + indxname + "/mode/2").value("write");
        ptr->nav(std::string("index/") + indxtype + "/" + indxname + "/type").value(indxtype);
        ptr->nav(std::string("index/") + indxtype + "/" + indxname + "/field").value(indxfield);
        return 0;
    }
    
    int LuaStorageFactory::_new_config(lua_State *L) {
        std::string dbname(luaL_checkstring(L, -1));
        lj::BSONNode *ptr = new lj::BSONNode();
        ptr->nav("main/compare").value(std::string("int64"));
        ptr->nav("main/file").value(std::string("db_") + dbname + ".tcb");
        ptr->nav("main/mode/0").value("create");
        ptr->nav("main/mode/1").value("read");
        ptr->nav("main/mode/2").value("write");
        ptr->nav("main/type").value("tree");
        ptr->nav("index/tree");
        ptr->nav("index/text");
        ptr->nav("index/tag");
        ptr->nav("index/hash");
        Lunar<lj::BSONNode>::push(L, ptr, true);
        return 1;
    }
    
    int LuaStorageFactory::_save_config(lua_State *L) {
        std::string dbname(luaL_checkstring(L, -2));
        lj::BSONNode *ptr = Lunar<lj::BSONNode>::check(L, -1);
        std::string dbfile(_dbdir);
        if(dbname.size() > 1 && dbname[dbname.size() - 1] == '/')
            dbfile.append(dbname);
        else
            dbfile.append("/").append(dbname);
        dbfile.append("/config");
        ptr->save(dbfile);
        return 0;
    }
    int LuaStorageFactory::_load_config(lua_State *L) {
        std::string dbname(luaL_checkstring(L, -1));
        std::string dbfile(_dbdir);
        if(dbname.size() > 1 && dbname[dbname.size() - 1] == '/')
            dbfile.append(dbname);
        else
            dbfile.append("/").append(dbname);
        dbfile.append("/config");
        lj::BSONNode *ptr = new lj::BSONNode();
        ptr->load(dbfile);
        Lunar<lj::BSONNode>::push(L, ptr, true);
        return 1;
    }
};