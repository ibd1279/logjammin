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
#include "lj/Base64.h"
#include "lj/Logger.h"
#include "lj/Storage.h"
#include "lj/Time_tracker.h"
#include "build/default/config.h"

#include <string>
#include <sstream>

using lj::Log;

namespace
{
    lj::Bson* get_connection_config(std::string dbfile)
    {
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
            lj::Bson* handlers = db_ptr->real_storage(*config).configuration()->path("handler");
            for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter2 = handlers->to_map().begin();
                 handlers->to_map().end() != iter2;
                 ++iter2)
            {
                if (!iter2->second->exists())
                {
                    Log::debug.log("Skipping [%s] for [%s]") << iter2->first << dbname << Log::end;
                    continue;
                }
                
                std::string event_name(dbname);
                event_name.append("__").append(iter2->first);
                lua_pushstring(L, event_name.c_str());

                logjamd::Function_buffer state(iter2->second->size());

                uint32_t sz = 0;
                lj::Bson::Binary_type t = lj::Bson::k_bin_function;
                const char* tmp = lj::bson_as_binary(*(iter2->second),
                                                     &t,
                                                     &sz);
                state.copy(tmp, sz);
                
                if (lua_load(L, &logjamd::function_reader, &state, event_name.c_str()))
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
    //! Put the environment table for this identifier ontop of the stack.
    /*!
     \param L The Root lua state.
     \param identifier The connection id.
     */
    int sandbox_push(lua_State* L)
    {
        lua_getglobal(L, "environment_cache"); // {ec}
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1); // {}
            lua_newtable(L); // {ec}
            lua_pushvalue(L, -1); // {ec, ec}
            lua_setglobal(L, "environment_cache"); // {ec}
        }
        lua_pushthread(L); // {ec, thread}
        lua_gettable(L, -2); // {ec, t}
        if (lua_isnil(L, -1))
        {
            lua_pop(L, 1); // {ec}
            lua_newtable(L); // {ec, t}
            lua_pushthread(L); // {ec, t, thread}
            lua_pushvalue(L, -2); // {ec, t, name, t}
            lua_settable(L, -4); // {ec, t}
            lua_pushvalue(L, -1); // {ec, t, t}
            lua_pushstring(L, "__index"); // {ec, t, t, __index}
            lua_pushvalue(L, LUA_GLOBALSINDEX); // {ec, t, t, __index, _G}
            lua_settable(L, -3); // {ec, t, t}
            lua_setmetatable(L, -2); // {ec, t}
        }
        lua_replace(L, -2); // {t}
        return 1;
    }

    int sandbox_get(lua_State* L, const std::string& key)
    {
        // {}
        sandbox_push(L); // {sandbox}
        lua_pushlstring(L, key.c_str(), key.size()); // {sandbox, key}
        lua_gettable(L, -2); // {sandbox, value}
        lua_replace(L, -2); // {value}
        return 1;
    }

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

    int sc_new(lua_State* L)
    {
        std::string dbname(lua_to_string(L, -1));
        lj::Bson* ptr = new lj::Bson();
        Lunar<Lua_bson>::push(L, new Lua_bson(ptr, true), true);
        
        lj::storage_config_init(*ptr, dbname);
        return 1;
    }
    
    int sc_add_index(lua_State* L)
    {
        std::string indxcomp(lua_to_string(L, -1));
        std::string indxfield(lua_to_string(L, -2));
        std::string indxtype(lua_to_string(L, -3));
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -4);
        
        lj::storage_config_add_index(ptr->real_node(), indxtype, indxfield, indxcomp);
        return 0;
    }
    
    int sc_add_nested_field(lua_State* L)
    {
        std::string field(lua_to_string(L, -1));
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -2);
        
        lj::storage_config_add_subfield(ptr->real_node(), field);
        return 0;
    }
    
    int sc_save(lua_State* L)
    {
        // Get the data directory.
        lua_getglobal(L, "lj__config");
        lj::Bson& server_config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        lj::storage_config_save(ptr->real_node(), server_config);
        return 0;
    }
    
    int sc_load(lua_State* L)
    {
        // Get the data directory.
        lua_getglobal(L, "lj__config");
        lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        lj::Bson& tmp = config.nav("data_directory");
        
        std::string name(lua_to_string(L, -1));
        lj::Bson* ptr = lj::storage_config_load(name, lj::bson_as_string(tmp));
        Lunar<Lua_bson>::push(L, new Lua_bson(ptr, true), true);
        return 1;
    }
    
    int sc_add_handler(lua_State* L)
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
    
    int sc_remove_handler(lua_State* L)
    {
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -2);
        std::string event("handler/");
        event.append(lua_to_string(L, -1));
        ptr->real_node().nav(event).destroy();
        return 0;
    }

    namespace lua
    {
        int server_port(lua_State* L)
        {
            // {arg}
            sandbox_get(L, "lj__config"); // {arg, config}
            lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
            int port = lua_tointeger(L, -2);
            lua_pop(L, 2); // {}

            // where do we save this file when done?
            lj::Bson& configfile = config.nav("server/configfile");

            // Set the new value.
            config.set_child("server/port", lj::bson_new_int64(port));

            // Save the config file to disk.
            lj::bson_save(config, lj::bson_as_string(configfile));

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%d]. New setting will take effect when the server is restarted.")
                    << "server/port" << port << lj::Log::end;
            return 0;
        }

        int server_directory(lua_State* L)
        {
            // {arg}
            sandbox_get(L, "lj__config"); // {arg, config}
            lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
            std::string directory = lua_to_string(L, -2);
            lua_pop(L, 2); // {}

            // where do we save this file when done?
            lj::Bson& configfile = config.nav("server/configfile");

            // Set the new value.
            config.set_child("server/directory", lj::bson_new_string(directory));

            // Save the config file to disk.
            lj::bson_save(config, lj::bson_as_string(configfile));

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s]. New setting will take effect when the server is restarted.")
                    << "server/directory" << directory << lj::Log::end;
            return 0;
        }

        int server_id(lua_State* L)
        {
            // {arg}
            sandbox_get(L, "lj__config"); // {arg, config}
            lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
            std::string server_id = lua_to_string(L, -2);
            lua_pop(L, 2); // {}

            // where do we save this file when done?
            lj::Bson& configfile = config.nav("server/configfile");

            // Set the new value.
            config.set_child("server/id", lj::bson_new_string(server_id));

            // Save the config file to disk.
            lj::bson_save(config, lj::bson_as_string(configfile));

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s]. New setting will take effect when the server is restarted.")
                    << "server/id" << server_id << lj::Log::end;
            return 0;
        }

        int storage_autoload(lua_State* L)
        {
            // {cmd, storage}
            sandbox_get(L, "lj__config"); // {cmd, storage, config}
            lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
            std::string storage = lua_to_string(L, -2);
            std::string command = lua_to_string(L, -3);
            lua_pop(L, 3); // {}

            // where do we save this file when done?
            lj::Bson& configfile = config.nav("server/configfile");

            // create a pointer into the config for ease of reference.
            lj::Bson* ptr = config.path("storage/autoload");

            if (command.compare("rm") == 0)
            {
                // loop over the bson list and remove this
                // value if you find it.
                for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = ptr->to_map().begin();
                     ptr->to_map().end() != iter;
                     ++iter)
                {
                    if (lj::bson_as_string(*(iter->second)).compare(storage) == 0)
                    {
                        ptr->set_child(iter->first, NULL);
                    }
                }
            }
            else if (command.compare("add") == 0)
            {
                // Only add the storage if it doesn't already exist.
                if (lj::bson_as_value_string_set(*ptr).count(storage) == 0)
                {
                    ptr->push_child("", lj::bson_new_string(storage));
                }
            }

            // Save the config file to disk.
            lj::bson_save(config, lj::bson_as_string(configfile));

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s %s]. New setting will take effect when the server is restarted.")
                    << "storage/autoload" << command << storage << lj::Log::end;
            return 0;
        }

        int replication_peer(lua_State* L)
        {
            // {cmd, peer}
            sandbox_get(L, "lj__config"); // {cmd, peer, config}
            lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
            std::string peer = lua_to_string(L, -2);
            std::string command = lua_to_string(L, -3);
            lua_pop(L, 3); // {}

            // where do we save this file when done?
            lj::Bson& configfile = config.nav("server/configfile");

            // create a pointer into the config for ease of reference.
            lj::Bson* ptr = config.path("replication/peer");

            if (command.compare("rm") == 0)
            {
                // loop over the bson list and remove this
                // value if you find it.
                for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = ptr->to_map().begin();
                     ptr->to_map().end() != iter;
                     ++iter)
                {
                    if (lj::bson_as_string(*(iter->second)).compare(peer) == 0)
                    {
                        ptr->set_child(iter->first, NULL);
                    }
                }
            }
            else if (command.compare("add") == 0)
            {
                // Only add the storage if it doesn't already exist.
                if (lj::bson_as_value_string_set(*ptr).count(peer) == 0)
                {
                    ptr->push_child("", lj::bson_new_string(peer));
                }
            }

            // Save the config file to disk.
            lj::bson_save(config, lj::bson_as_string(configfile));

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s %s]. New setting will take effect when the server is restarted.")
                    << "replication/peer" << command << peer << lj::Log::end;
            return 0;
        }

        int logging_level(lua_State* L)
        {
            // {cmd, peer}
            sandbox_get(L, "lj__config"); // {cmd, peer, config}
            lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
            bool enabled = lua_toboolean(L, -2);
            std::string level = lua_to_string(L, -3);
            lua_pop(L, 3); // {}

            // where do we save this file when done?
            lj::Bson& configfile = config.nav("server/configfile");

            // set the value.
            config.nav("logging").set_child(level, lj::bson_new_boolean(enabled));

            // Save the config file to disk.
            lj::bson_save(config, lj::bson_as_string(configfile));

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s/%s] config setting changed to [%s]. New setting will take effect when the server is restarted.")
                    << "logging" << level << enabled << lj::Log::end;
            return 0;
        }

        int send_item(lua_State* L)
        {        
            // {item}
            sandbox_get(L, "lj__response"); // {item, response}
            Lua_bson* response = Lunar<Lua_bson>::check(L, -1);
            Lua_bson* item = Lunar<Lua_bson>::check(L, -2);
            lua_pop(L, 2); // {}
            response->real_node().push_child("item", new lj::Bson(item->real_node()));
            return 0;
        }

        int print(lua_State* L)
        {
            // {item}
            sandbox_get(L, "lj__response"); // {arg, response}
            Lua_bson* response = Lunar<Lua_bson>::check(L, -1);
            std::string arg = lua_to_string(L, -2);
            lua_pop(L, 2); // {}
            response->real_node().push_child("lj__output", lj::bson_new_string(arg));
            return 0;
        }
    }; // namespace lua

    void register_config_api(lua_State* L)
    {
        // Load the Bson class into lua.
        Lunar<logjamd::Lua_bson>::Register(L);
        
        // load standard lj functions.
        lua_register(L, "send_set",
                     &send_set);
        lua_register(L, "send_item",
                     &logjamd::lua::send_item);
        lua_register(L, "print",
                     &logjamd::lua::print);

        // load the storage configuration functions.
        lua_register(L, "lj__server_port",
                     &logjamd::lua::server_port);
        lua_register(L, "lj__server_directory",
                     &logjamd::lua::server_directory);
        lua_register(L, "lj__server_id",
                     &logjamd::lua::server_id);
        lua_register(L, "lj__storage_autoload",
                     &logjamd::lua::storage_autoload);
        lua_register(L, "lj__replication_peer",
                     &logjamd::lua::replication_peer);
        lua_register(L, "lj__logging_level",
                     &logjamd::lua::logging_level);
    }

    void logjam_lua_init(lua_State* L, lj::Bson* config) {
        // register the configuration api.
        register_config_api(L);

        // Register the object model.
        Lunar<logjamd::Lua_record_set>::Register(L);
        Lunar<logjamd::Lua_storage>::Register(L);
        
        // load storage config functions.
        lua_register(L, "sc_new", &sc_new);
        lua_register(L, "sc_save", &sc_save);
        lua_register(L, "sc_load", &sc_load);
        lua_register(L, "sc_add_index", &sc_add_index);
        lua_register(L, "sc_add_nested_field", &sc_add_nested_field);
        lua_register(L, "sc_add_handler", &sc_add_handler);
        lua_register(L, "sc_remove_handler", &sc_remove_handler);
        
        // load the configuration file into the lua context.
        Lunar<logjamd::Lua_bson>::push(L, new Lua_bson(config, false), true);
        lua_setglobal(L, "lj__config");

        // Build the default storage object.
        push_default_storage(L, config);
        
        // Server ID.
        lua_pushinteger(L, rand());
        lua_setglobal(L, "server_id");
    }
    
    
    //=====================================================================
    // logjam global functions.
    //=====================================================================
    
    
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

    int connection_config_add_replication_peer(lua_State* L)
    {
        lj::Bson* peer = new lj::Bson();
        peer->set_child("host", lj::bson_new_string(lua_to_string(L, -3)));
        peer->set_child("port", lj::bson_new_uint64(lua_tointeger(L, -2)));
        Lua_bson* wrapped_config = Lunar<Lua_bson>::check(L, -1);
        lj::Bson& config = wrapped_config->real_node();

        lj::Bson& replication_config = config.nav("replication/peers");
        if (lj::bson_as_value_string_set(replication_config).count(lj::bson_as_string(*peer)) == 0)
        {
            replication_config.push_child("", peer);
        }
        else
        {
            delete peer;
        }
        return 0;
    }

    int connection_config_remove_replication_peer(lua_State* L)
    {
        lj::Bson* peer = new lj::Bson();
        peer->set_child("host", lj::bson_new_string(lua_to_string(L, -3)));
        peer->set_child("port", lj::bson_new_uint64(lua_tointeger(L, -2)));
        Lua_bson* wrapped_config = Lunar<Lua_bson>::check(L, -1);
        lj::Bson& config = wrapped_config->real_node();

        lj::Bson& replication_config = config.nav("replication/peers");
        for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = replication_config.to_map().begin();
             replication_config.to_map().end() != iter;
             ++iter)
        {
            if (lj::bson_as_string(*(iter->second)).compare(lj::bson_as_string(*peer)) == 0)
            {
                replication_config.set_child(iter->first, NULL);
            }
        }
        return 0;
    }

    
    void get_event(lua_State* L, const std::string& db_name, const std::string& event)
    {
        std::string event_key(db_name);
        event_key.append("__").append(event);
        lua_getglobal(L, "db_events");
        lua_pushstring(L, event_key.c_str());
        lua_gettable(L, -2); // db_events, func.
        lua_remove(L, -2); // func
    }
        
    const std::string push_replication_record(lua_State* L, const lj::Bson& b)
    {
        lua_pushstring(L, "o"); // {a}
        lua_pushinteger(L, rand() % 100); // {a, b}
        lua_pushstring(L, "_"); // {a, b, c}
        lua_pushinteger(L, rand()); // {a, b, c, d}
        lua_concat(L, 4); // {record_id}
        std::string name(lua_to_string(L, -1));
        lua_pop(L, 1); // {}

        sandbox_get(L, "lj__replication"); // {replication}
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1); // {}
        ptr->real_node().set_child(name, new lj::Bson(b));
        
        return name;
    }
    
    void push_replication_command(lua_State* L,
                                  const std::string& action,
                                  const std::string& dbname,
                                  const std::string& obj)
    {
        sandbox_get(L, "lj__replication"); // {replication}
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1); // {}
        
        std::string cmd("replication_");
        cmd.append(action);
        cmd.append("('");
        cmd.append(dbname);
        cmd.append("', '");
        cmd.append(obj);
        cmd.append("')");
        
        std::string script(lj::bson_as_string(ptr->real_node().nav("cmd")));
        script.append("\n").append(cmd);
        ptr->real_node().set_child("cmd", lj::bson_new_string(script));
    }
    
    int send_set(lua_State *L)
    {
        // {record_set}
        lj::Time_tracker timer;
        timer.start();

        sandbox_get(L, "lj__response"); // {record_set, response}

        // Get what we need from lua's stack
        Lua_record_set* filter = Lunar<Lua_record_set>::check(L, -2);
        Lua_bson* response = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 2); // {}
        
        // Put the command parts together to make a full string.
        // String is used for recording the cost in the response.
        std::string cmd("send_set(");
        for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = filter->costs().to_map().begin();
             filter->costs().to_map().end() != iter;
             ++iter)
        {
            cmd.append(lj::bson_as_string(*(*iter).second->path("cmd")));
            cmd.append(":");
        }
        cmd.erase(cmd.size() - 1).append(")");
        
        // copy the costs, incase they use the result set more than once.
        lj::Bson* cost_data = new lj::Bson(filter->costs());
        
        // Get the items for the result set.
        lj::Bson* items = new lj::Bson();
        filter->real_set().items_raw(*items);
        
        // Put it all together.
        lj::Bson* result = new lj::Bson();
        result->set_child("cmd", lj::bson_new_string(cmd));
        result->set_child("costs", cost_data);
        result->set_child("items", items);

        // Put it on the response.
        response->real_node().push_child("results", result);
        
        // Add the last cost
        timer.stop();
        cost_data->push_child("", lj::bson_new_cost("send_set",
                                                    timer.elapsed(),
                                                    filter->real_set().size(),
                                                    filter->real_set().size()));
        return 0;
    }
};
