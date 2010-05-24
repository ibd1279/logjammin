/*
 *  Lua_storage.cpp
 *  logjammin
 *
 *  Created by Jason Watson on 5/24/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "logjamd/Lua_storage.h"

#include "build/default/config.h"
#include "logjamd/logjamd_lua.h"
#include "lj/Storage_factory.h"

#include <string>
#include <sstream>

// XXX This should be moved somewhere for portability.
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

namespace logjamd
{
    const char Lua_storage::LUNAR_CLASS_NAME[] = "Storage";
    Lunar<Lua_storage>::RegType Lua_storage::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Lua_storage, all),
    LUNAR_MEMBER_METHOD(Lua_storage, none),
    LUNAR_MEMBER_METHOD(Lua_storage, at),
    LUNAR_MEMBER_METHOD(Lua_storage, place),
    LUNAR_MEMBER_METHOD(Lua_storage, remove),
    {0, 0, 0}
    };
    
    Lua_storage::Lua_storage(const std::string& dbname) : storage_(NULL)
    {
        storage_ = lj::Storage_factory::produce(dbname);
    }
    
    Lua_storage::Lua_storage(lua_State* L) : storage_(NULL)
    {
        std::string dbname(lua_to_string(L, -1));
        storage_ = lj::Storage_factory::produce(dbname);
    }
    
    Lua_storage::~Lua_storage()
    {
    }
    
    int Lua_storage::all(lua_State* L)
    {
        lj::Record_set* ptr = real_storage().all().release();
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int Lua_storage::none(lua_State* L)
    {
        lj::Record_set* ptr = real_storage().none().release();
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int Lua_storage::at(lua_State* L)
    {
        lj::Record_set* ptr = real_storage().at(luaL_checkint(L, -1)).release();
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int Lua_storage::place(lua_State* L)
    {
        Lua_bson_node* ptr = Lunar<Lua_bson_node>::check(L, -1);
        try
        {
            real_storage().place(ptr->real_node());
        }
        catch(lj::Exception* ex)
        {
            luaL_error(L, "Unable to place content. %s", ex->to_string().c_str());
        }
        return 0;
    }
    
    int Lua_storage::remove(lua_State* L)
    {
        Lua_bson_node* ptr = Lunar<Lua_bson_node>::check(L, -1);
        real_storage().remove(ptr->real_node());
        return 0;
    }
    
    int storage_config_new(lua_State *L) {
        std::string dbname(lua_to_string(L, -1));
        lj::Bson *ptr = new lj::Bson();
        ptr->set_child("main/compare", lj::bson_new_string("int64"));
        ptr->set_child("main/file", lj::bson_new_string(std::string("db_") + dbname + ".tcb"));
        ptr->push_child("main/mode", lj::bson_new_string("create"));
        ptr->push_child("main/mode", lj::bson_new_string("read"));
        ptr->push_child("main/mode", lj::bson_new_string("write"));
        ptr->set_child("main/type", lj::bson_new_string("tree"));
        ptr->nav("main/unique");
        ptr->nav("index/tree");
        ptr->nav("index/text");
        ptr->nav("index/tag");
        ptr->nav("index/hash");
        Lunar<Lua_bson_node>::push(L, new Lua_bson_node(ptr, true), true);
        return 1;
    }
    
    int storage_config_save(lua_State *L) {
        std::string dbname(lua_to_string(L, -2));
        Lua_bson_node *ptr = Lunar<Lua_bson_node>::check(L, -1);
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
        lj::bson_save(ptr->real_node(), dbfile);
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
        lj::Bson *ptr = lj::bson_load(dbfile);
        Lunar<Lua_bson_node>::push(L, new Lua_bson_node(ptr, true), true);
        return 1;
    }
    
    int storage_config_add_index(lua_State *L) {
        std::string indxcomp(lua_to_string(L, -1));
        std::string indxfield(lua_to_string(L, -2));
        std::string indxname(lua_to_string(L, -3));
        std::string indxtype(lua_to_string(L, -4));
        Lua_bson_node *ptr = Lunar<Lua_bson_node>::check(L, -5);
        ptr->real_node().set_child(std::string("index/") + indxtype + "/" + indxname + "/compare", lj::bson_new_string(indxcomp));
        ptr->real_node().set_child(std::string("index/") + indxtype + "/" + indxname + "/file", lj::bson_new_string(std::string("index.") + indxname + "." + indxtype + ".tc"));
        ptr->real_node().push_child(std::string("index/") + indxtype + "/" + indxname + "/mode", lj::bson_new_string("create"));
        ptr->real_node().push_child(std::string("index/") + indxtype + "/" + indxname + "/mode", lj::bson_new_string("read"));
        ptr->real_node().push_child(std::string("index/") + indxtype + "/" + indxname + "/mode", lj::bson_new_string("write"));
        ptr->real_node().set_child(std::string("index/") + indxtype + "/" + indxname + "/type", lj::bson_new_string(indxtype));
        ptr->real_node().set_child(std::string("index/") + indxtype + "/" + indxname + "/field", lj::bson_new_string(indxfield));
        ptr->real_node().set_child(std::string("index/") + indxtype + "/" + indxname + "/children", lj::bson_new_boolean(false));
        return 0;
    }
    
    int storage_config_add_nested_field(lua_State *L) {
        std::string field(lua_to_string(L, -1));
        Lua_bson_node *ptr = Lunar<Lua_bson_node>::check(L, -2);
        
        std::set<std::string> allowed(lj::bson_as_value_string_set(ptr->real_node().nav("main/nested")));
        allowed.insert(field);
        
        lj::Bson* n = ptr->real_node().path("main/nested");
        n->destroy();
        int h = 0;
        for(std::set<std::string>::const_iterator iter = allowed.begin();
            iter != allowed.end();
            ++iter) {
            std::ostringstream buf;
            buf << h++;
            n->set_child(buf.str(), lj::bson_new_string(*iter));
        }
        return 0;
    }
}; // namespace logjamd
