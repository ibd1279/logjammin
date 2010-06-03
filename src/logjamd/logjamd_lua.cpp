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
#include "logjamd/Lua_record_set.h"
#include "logjamd/Lua_storage.h"
#include "lj/base64.h"
#include "lj/Logger.h"
#include "lj/Time_tracker.h"
#include "build/default/config.h"

#include <string>
#include <sstream>

// XXX This should be moved somewhere for portability.
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

using lj::Log;

namespace
{
    struct Function_buffer
    {
        char* const buf;
        char* cur;
        char* const max;
        size_t size;
        
        Function_buffer(size_t sz) : buf(new char[sz + 1]), cur(buf), max(buf + sz + 1)
        {
        }
        
        ~Function_buffer()
        {
            delete[] buf;
        }
        
        int copy(const void* source, size_t sz)
        {
            if (cur + sz >= max)
            {
                return 1;
            }
            
            memcpy(cur, source, sz);
            cur += sz;
            return 0;
        }
    private:
        Function_buffer(const Function_buffer&);
    };
    
    int function_writer(lua_State*,
                        const void* p,
                        size_t sz,
                        void* ud)
    {
        Function_buffer* ptr = static_cast<Function_buffer*>(ud);
        return ptr->copy(p, sz);
    }

    const char* function_reader(lua_State* L,
                                void* ud,
                                size_t* sz)
    {
        Function_buffer* ptr = static_cast<Function_buffer*>(ud);
        if (ptr->cur >= ptr->max)
        {
            *sz = 0;
            return 0;
        }
        else
        {
            const char* bytes = ptr->buf;
            *sz = ptr->cur - ptr->buf;
            ptr->cur = ptr->max;
            return bytes;
        }
    }
    
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
        int db_table = lua_gettop(L);
        lua_newtable(L);
        int event_table = lua_gettop(L);
        lj::Bson* default_storage = config->path("default_storage");
        for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = default_storage->to_map().begin();
             default_storage->to_map().end() != iter;
             ++iter)
        {
            std::string dbname(lj::bson_as_string(*iter->second));
            lua_pushstring(L, dbname.c_str());
            logjamd::Lua_storage* db_ptr = new logjamd::Lua_storage(dbname);
            Lunar<logjamd::Lua_storage>::push(L, db_ptr, true);
            lua_settable(L, db_table);
            
            // Add some logic to load the event handlers.
            lj::Bson* handlers = db_ptr->real_storage().configuration()->path("handler");
            for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter2 = handlers->to_map().begin();
                 handlers->to_map().end() != iter2;
                 ++iter2)
            {
                std::string event_name(dbname);
                event_name.append("__").append(iter2->first);
                lua_pushstring(L, event_name.c_str());

                Function_buffer state(iter2->second->size());
                if (lj::Bson::k_string == iter2->second->type())
                {
                    std::string tmp(lj::bson_as_string(*(iter2->second)));
                    state.copy(tmp.c_str(), tmp.size());
                }
                else
                {
                    uint32_t sz = 0;
                    lj::Bson::Binary_type t = lj::Bson::k_bin_function;
                    const char* tmp = lj::bson_as_binary(*(iter2->second),
                                                         &t,
                                                         &sz);
                    state.copy(tmp, sz);
                }
                
                if (lua_load(L, &function_reader, &state, event_name.c_str()))
                {
                    Log::critical.log("Error %s") << lua_to_string(L, -1) << Log::end;
                    lua_pop(L, 2);
                }
                else
                {
                    lua_settable(L, event_table);
                }
            }
        }
        lua_setglobal(L, "db_events");
        lua_setglobal(L, "db");
    }
}; // namespace

namespace logjamd
{
    void register_logjam_functions(lua_State *L) {
        // Load object model.
        Lunar<logjamd::Lua_bson>::Register(L);
        Lunar<logjamd::Lua_record_set>::Register(L);
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
        lua_pushcfunction(L, &storage_config_add_handler);
        lua_setglobal(L, "sc_add_event_handler");
        
        // load standard query functions.
        lua_pushcfunction(L, &send_set);
        lua_setglobal(L, "send_set");
        lua_pushcfunction(L, &send_item);
        lua_setglobal(L, "send_item");
        
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
    
    int storage_config_new(lua_State* L) {
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
        Lunar<Lua_bson>::push(L, new Lua_bson(ptr, true), true);
        return 1;
    }
    
    int storage_config_save(lua_State* L) {
        std::string dbname(lua_to_string(L, -2));
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
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
    
    int storage_config_load(lua_State* L) {
        std::string dbname(lua_to_string(L, -1));
        std::string dbfile(DBDIR);
        if(dbname.size() > 1 && dbname[dbname.size() - 1] == '/')
            dbfile.append(dbname);
        else
            dbfile.append("/").append(dbname);
        dbfile.append("/config");
        lj::Bson* ptr = lj::bson_load(dbfile);
        Lunar<Lua_bson>::push(L, new Lua_bson(ptr, true), true);
        return 1;
    }
    
    int storage_config_add_index(lua_State* L) {
        std::string indxcomp(lua_to_string(L, -1));
        std::string indxfield(lua_to_string(L, -2));
        std::string indxname(lua_to_string(L, -3));
        std::string indxtype(lua_to_string(L, -4));
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -5);
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
    
    int storage_config_add_nested_field(lua_State* L) {
        std::string field(lua_to_string(L, -1));
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -2);
        
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
    
    int storage_config_add_handler(lua_State* L)
    {
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -3);
        std::string event("handler/");
        event.append(lua_to_string(L, -2));
        
        if (lua_isstring(L, -1))
        {
            lj::Bson* function = lj::bson_new_string(lua_to_string(L, -1));
            ptr->real_node().set_child(event, function);
        }
        else if (lua_isfunction(L, -1) && !lua_iscfunction(L, -1))
        {
            Function_buffer buffer(10 * 1024);
            lua_dump(L, &function_writer, &buffer);
            
            lj::Bson* function = lj::bson_new_binary(buffer.buf,
                                                     buffer.cur - buffer.buf,
                                                     lj::Bson::k_bin_function);
            ptr->real_node().set_child(event, function);
        }
        else
        {
            luaL_argerror(L, -1, "Expected string of lua, or a lua function.");
        }
        return 0;
        
    }
    
    int storage_config_remove_handler(lua_State* L)
    {
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -2);
        std::string event("handler/");
        event.append(lua_to_string(L, -1));
        ptr->real_node().nav(event).destroy();
        return 0;
    }
    
    int send_set(lua_State *L)
    {
        lj::Time_tracker timer;
        Lua_record_set* filter = Lunar<Lua_record_set>::check(L, -1);
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        
        timer.start();
        lj::Bson* d = new lj::Bson();
        node->real_node().push_child("results", d);
        filter->real_set().items_raw(*d);
        timer.stop();
        
        node->real_node().push_child("time/load_usecs",
                                     lj::bson_new_uint64(timer.elapsed()));
        
        return 0;
    }
    
    int send_item(lua_State* L)
    {        
        Lua_bson* item = Lunar<Lua_bson>::check(L, -1);
        lua_getglobal(L, "response");
        Lua_bson* response = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        
        response->real_node().push_child("item", new lj::Bson(item->real_node()));
        
        return 0;
    }
};