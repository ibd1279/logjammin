/*!
 \file logjamd_lua.cpp
 \brief Logjam server lua functions implementation.
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

#include "logjamd/logjamd_lua.h"
#include "logjamd/Lua_bson.h"
#include "logjamd/Lua_storage.h"
#include "lj/Logger.h"
#include "lj/Time_tracker.h"
#include "build/default/config.h"

#include <string>
#include <sstream>


using lj::Log;

namespace
{    
    lj::Bson* get_connection_config()
    {
        std::string dbfile(DBDIR);
        dbfile.append("/config");
        lj::Bson* ptr = lj::bson_load(dbfile);
        return ptr;
    }
    
    void push_default_storage(lua_State* L, lj::Bson* config)
    {
        lua_newtable(L);
        lj::Bson* default_storage = config->path("default_storage");
        for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = default_storage->to_map().begin();
             default_storage->to_map().end() != iter;
             ++iter)
        {
            lua_pushstring(L, lj::bson_as_string(*iter->second).c_str());
            logjamd::Lua_storage* ptr = new logjamd::Lua_storage(lj::bson_as_string(*iter->second));
            Lunar<logjamd::Lua_storage>::push(L, ptr, true);
            lua_settable(L, -3);
        }
        lua_setglobal(L, "db");
    }
}; // namespace

namespace logjamd
{
    void register_logjam_functions(lua_State *L) {
        // Load object model.
        Lunar<logjamd::Lua_bson>::Register(L);
        Lunar<logjamd::LuaStorageFilter>::Register(L);
        Lunar<logjamd::Lua_storage>::Register(L);
        
        // load connection config functions.
        lua_pushcfunction(L, &connection_config_load);
        lua_setglobal(L, "cc_load");
        lua_pushcfunction(L, &connection_config_save);
        lua_setglobal(L, "cc_save");
        lua_pushcfunction(L, &connection_config_add_default_storage);
        lua_setglobal(L, "cc_add_default_storage");
        lua_pushcfunction(L, &connection_config_remove_default_storage);
        lua_setglobal(L, "cc_remove_default_storage");
        
        // load storage config functions.
        lua_pushcfunction(L, &storage_config_new);
        lua_setglobal(L, "sc_new");
        lua_pushcfunction(L, &storage_config_save);
        lua_setglobal(L, "sc_save");
        lua_pushcfunction(L, &storage_config_load);
        lua_setglobal(L, "sc_load");
        lua_pushcfunction(L, &storage_config_add_index);
        lua_setglobal(L, "sc_add_index");
        lua_pushcfunction(L, &storage_config_add_nested_field);
        lua_setglobal(L, "sc_add_nested");
        
        // load standard query functions.
        lua_pushcfunction(L, &send_response);
        lua_setglobal(L, "send_response");
        
        // load the default storage.
        lj::Bson* config = get_connection_config();
        push_default_storage(L, config);
        
        // push the config into the global scope.
        Lunar<logjamd::Lua_bson>::push(L, new Lua_bson(config, true), true);
        lua_setglobal(L, "connection_config");
    }
    
    //=====================================================================
    // logjam global functions.
    //=====================================================================
    int connection_config_load(lua_State* L)
    {
        lj::Bson* ptr = get_connection_config();
        Lunar<Lua_bson>::push(L, new Lua_bson(ptr, true), true);
        return 1;
    }
    
    int connection_config_save(lua_State* L)
    {
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        std::string dbfile(DBDIR);
        dbfile.append("/config");
        lj::bson_save(ptr->real_node(), dbfile);
        return 0;
    }
    
    int connection_config_add_default_storage(lua_State* L)
    {
        std::string storage_name(lua_to_string(L, -2));
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        if (lj::bson_as_value_string_set(ptr->real_node()).count(storage_name) == 0)
        {
            ptr->real_node().push_child("default_storage", lj::bson_new_string(storage_name));
        }
        return 0;
    }
    
    int connection_config_remove_default_storage(lua_State* L)
    {
        std::string storage_name(lua_to_string(L, -2));
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        lj::Bson* default_storage = ptr->real_node().path("default_storage");
        for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = default_storage->to_map().begin();
             default_storage->to_map().end() != iter;
             ++iter)
        {
            if (lj::bson_as_string(*(iter->second)).compare(storage_name) == 0)
            {
                default_storage->set_child(iter->first, NULL);
            }
        }
        return 0;
    }
    
    int send_response(lua_State *L)
    {
        lj::Time_tracker timer;
        LuaStorageFilter* filter = Lunar<LuaStorageFilter>::check(L, -1);
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        
        timer.start();
        std::list<lj::Bson *> d;
        filter->real_filter().items_raw(d);
        for (std::list<lj::Bson *>::const_iterator iter = d.begin();
             iter != d.end();
             ++iter)
        {
            node->real_node().push_child("items", *iter);
        }
        timer.stop();
        node->real_node().push_child("time/load_usecs",
                                     lj::bson_new_uint64(timer.elapsed()));
        
        return 0;
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
    LUNAR_MEMBER_METHOD(LuaStorageFilter, size),
    {0, 0, 0}
    };
    
    LuaStorageFilter::LuaStorageFilter(lua_State *L) : _filter(NULL) {
        Lua_storage *ptr = Lunar<Lua_storage>::check(L, -1);
        _filter = ptr->real_storage().none().release();
    }
    
    LuaStorageFilter::LuaStorageFilter(lj::Record_set *filter) : _filter(filter) {
    }
    
    LuaStorageFilter::~LuaStorageFilter() {
        if(_filter)
            delete _filter;
    }
    
    int LuaStorageFilter::mode_and(lua_State *L) {
        _filter->set_operation(lj::Record_set::k_intersection);
        Lunar<LuaStorageFilter>::push(L, this, false);
        return 1;
    }
    
    int LuaStorageFilter::mode_or(lua_State *L) {
        _filter->set_operation(lj::Record_set::k_union);
        Lunar<LuaStorageFilter>::push(L, this, false);
        return 1;
    }
    
    int LuaStorageFilter::filter(lua_State *L) {
        lj::Time_tracker timer;
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        timer.start();
        
        std::string field(lua_to_string(L, -2));
                if(lua_isstring(L, -1)) {
            std::string val(lua_to_string(L, -1));
            lj::Record_set* ptr = _filter->equal(field, val.c_str(), val.size()).release();
            Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        } else {
            Lua_bson* n = Lunar<Lua_bson>::check(L, -1);
            char* bson = n->real_node().to_binary();
            lj::Record_set* ptr = NULL;
            if(lj::bson_type_is_quotable(n->real_node().type())) {
                ptr = _filter->equal(field, bson + 4, n->real_node().size() - 5).release();
            } else {
                ptr = _filter->equal(field, bson, n->real_node().size()).release();
            }
            delete[] bson;
            Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        }
        timer.stop();
        node->real_node().push_child("time/query_usecs", 
                                     lj::bson_new_uint64(timer.elapsed()));
        return 1;
    }

    int LuaStorageFilter::search(lua_State *L) {
        std::string field(lua_to_string(L, -2));
        std::string val(lua_to_string(L, -1));
        lj::Record_set* ptr = _filter->contains(field, val).release();
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);
        return 1;
    }
    
    int LuaStorageFilter::tagged(lua_State *L) {
        lj::Time_tracker timer;
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        timer.start();
        
        std::string field(lua_to_string(L, -2));
        std::string val(lua_to_string(L, -1));
        lj::Record_set* ptr = _filter->tagged(field, val).release();
        Lunar<LuaStorageFilter>::push(L, new LuaStorageFilter(ptr), true);

        timer.stop();
        node->real_node().push_child("time/query_usecs", 
                                     lj::bson_new_uint64(timer.elapsed()));
        return 1;
    }
    
    int LuaStorageFilter::records(lua_State *L) {
        std::list<lj::Bson *> d;
        int h = 0;
        _filter->items(d);
        lua_newtable(L);
        for(std::list<lj::Bson *>::const_iterator iter = d.begin();
            iter != d.end();
            ++iter) {
            Lunar<Lua_bson>::push(L, new Lua_bson(*iter, true), true);
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
        Lunar<Lua_bson>::push(L, new Lua_bson(d, true), true);
        return 1;
    }
    
    int LuaStorageFilter::size(lua_State *L) {
        lua_pushinteger(L, _filter->size());
        return 1;
    }
};