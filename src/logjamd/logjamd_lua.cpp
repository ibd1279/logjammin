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
#include "lj/Storage_factory.h"
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
        lj::Bson* default_storage = config->path("storage/autoload");
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

    int sc_load(lua_State* L)
    {
        // Get the data directory.
        lua_getglobal(L, "lj__config");
        lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        
        std::string name(lua_to_string(L, -1));
        lj::Bson* ptr = lj::storage_config_load(name, config);
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
        namespace {
            void util_persist_config(lua_State* L, const lj::Bson& config)
            {
                // Disk save first, incase of failure.
                const lj::Bson& configfile = config.nav("server/configfile");
                lj::bson_save(config, lj::bson_as_string(configfile));

                // environment next
                logjamd::Lua_bson* wrapped_config = new logjamd::Lua_bson(new lj::Bson(config), true);
                sandbox_push(L); // {env}
                Lunar<logjamd::Lua_bson>::push(L, wrapped_config, true); // {env, cfg}
                lua_setfield(L, -2, "lj__config"); // {env}
                lua_pop(L, 1); // {}
            }

            std::string util_server_dir(const lj::Bson& config)
            {
                const lj::Bson& directory = config.nav("server/directory");
                return lj::bson_as_string(directory);
            }
        }; // namespace logjamd::lua::(anonymous)

        int server_port(lua_State* L)
        {
            // {arg}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            int port = lua_tointeger(L, -1);
            lua_pop(L, 1); // {}

            // Set the new value.
            config.set_child("server/port", lj::bson_new_int64(port));

            // Save the config to disk, and update env.
            util_persist_config(L, config);

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%d]. New setting will take effect when the server is restarted.")
                    << "server/port" << port << lj::Log::end;
            return 0;
        }

        int server_directory(lua_State* L)
        {
            // {arg}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string directory = lua_to_string(L, -1);
            lua_pop(L, 1); // {}

            // Set the new value.
            config.set_child("server/directory", lj::bson_new_string(directory));

            // Save the config file to disk.
            util_persist_config(L, config);

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s]. New setting will take effect when the server is restarted.")
                    << "server/directory" << directory << lj::Log::end;
            return 0;
        }

        int server_id(lua_State* L)
        {
            // {arg}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string server_id = lua_to_string(L, -1);
            lua_pop(L, 1); // {}

            // Set the new value.
            config.set_child("server/id", lj::bson_new_string(server_id));

            // Save the config file to disk.
            util_persist_config(L, config);

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s]. New setting will take effect when the server is restarted.")
                    << "server/id" << server_id << lj::Log::end;
            return 0;
        }

        int storage_autoload(lua_State* L)
        {
            // {cmd, storage}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string storage = lua_to_string(L, -1);
            std::string command = lua_to_string(L, -2);
            lua_pop(L, 2); // {}

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
            util_persist_config(L, config);

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s %s]. New setting will take effect when the server is restarted.")
                    << "storage/autoload" << command << storage << lj::Log::end;
            return 0;
        }

        int replication_peer(lua_State* L)
        {
            // {cmd, peer}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string peer = lua_to_string(L, -1);
            std::string command = lua_to_string(L, -2);
            lua_pop(L, 2); // {}

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
            util_persist_config(L, config);

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s] config setting changed to [%s %s]. New setting will take effect when the server is restarted.")
                    << "replication/peer" << command << peer << lj::Log::end;
            return 0;
        }

        int logging_level(lua_State* L)
        {
            // {level, enabled}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            bool enabled = lua_toboolean(L, -1);
            std::string level = lua_to_string(L, -2);
            lua_pop(L, 2); // {}

            // set the value.
            config.nav("logging").set_child(level, lj::bson_new_boolean(enabled));

            // Save the config file to disk.
            util_persist_config(L, config);

            // Write a log entry for the config change.
            lj::Log::alert.log("[%s/%s] config setting changed to [%s]. New setting will take effect when the server is restarted.")
                    << "logging" << level << enabled << lj::Log::end;
            return 0;
        }

        int storage_init(lua_State* L)
        {
            // {name}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string storage_name = lua_to_string(L, -1);
            lua_pop(L, 1); // {}

            lj::Bson storage_config;
            lj::storage_config_init(storage_config, storage_name);
            lj::storage_config_save(storage_config, config);
            lj::Storage_factory::recall(storage_name, config);

            return 0;
        }

        int storage_index(lua_State* L)
        {
            // {storage, field, type, compare}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string index_comparison(lua_to_string(L, -1));
            std::string index_type(lua_to_string(L, -2));
            std::string index_field(lua_to_string(L, -3));
            std::string storage_name(lua_to_string(L, -4));
            lua_pop(L, 4); // {}

            lj::Bson* storage_config = lj::storage_config_load(storage_name, config);
            lj::storage_config_add_index(*storage_config,
                                         index_type,
                                         index_field,
                                         index_comparison);
            lj::storage_config_save(*storage_config, config);
            lj::Storage_factory::recall(storage_name, config);
            delete storage_config;

            return 0;
        }

        int storage_subfield(lua_State* L)
        {
            // {storage, field}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string field(lua_to_string(L, -1));
            std::string storage_name(lua_to_string(L, -2));
            lua_pop(L, 2); // {}

            lj::Bson* storage_config = lj::storage_config_load(storage_name, config);
            lj::storage_config_add_subfield(*storage_config, field);
            lj::storage_config_save(*storage_config, config);
            lj::Storage_factory::recall(storage_name, config);
            delete storage_config;

            return 0;
        }

        int send_item(lua_State* L)
        {        
            // {item}
            sandbox_get(L, "lj__response"); // {item, response}
            Lua_bson* response = Lunar<logjamd::Lua_bson>::check(L, -1);
            Lua_bson* item = Lunar<logjamd::Lua_bson>::check(L, -2);
            lua_pop(L, 2); // {}
            response->real_node().push_child("item", new lj::Bson(item->real_node()));
            return 0;
        }

        int print(lua_State* L)
        {
            // {item}
            sandbox_get(L, "lj__response"); // {arg, response}
            Lua_bson* response = Lunar<logjamd::Lua_bson>::check(L, -1);
            std::string arg = lua_to_string(L, -2);
            lua_pop(L, 2); // {}
            response->real_node().push_child("lj__output", lj::bson_new_string(arg));
            return 0;
        }
    }; // namespace lua

    void register_config_api(lua_State* L, lj::Bson* config)
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

        // Push the configuration onto the stack for closures.
        Lunar<logjamd::Lua_bson>::push(L, new logjamd::Lua_bson(config, false), true); // {cfg}

        // load the server configuration functions.
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::server_port, 1); // {cfg, func}
        lua_setglobal(L, "lj__server_port"); // {cfg}
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::server_directory, 1); // {cfg, func}
        lua_setglobal(L, "lj__server_directory"); // {cfg}
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::server_id, 1); // {cfg, func}
        lua_setglobal(L, "lj__server_id"); // {cfg}
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::storage_autoload, 1); // {cfg, func}
        lua_setglobal(L, "lj__storage_autoload"); // {cfg}
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::replication_peer, 1); // {cfg, func}
        lua_setglobal(L, "lj__replication_peer"); // {cfg}
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::logging_level, 1); // {cfg, func}
        lua_setglobal(L, "lj__logging_level"); // {cfg}

        // load the storage configuration functions.
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::storage_init, 1); // {cfg, func}
        lua_setglobal(L, "lj_storage_init"); // {cfg}
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::storage_index, 1); // {cfg, func}
        lua_setglobal(L, "lj_storage_index"); // {cfg}
        lua_pushvalue(L, -1); // {cfg, cfg}
        lua_pushcclosure(L, &logjamd::lua::storage_subfield, 1); // {cfg, func}
        lua_setglobal(L, "lj_storage_subfield"); // {cfg}

        lua_pop(L, 1); // {}
    }

    void logjam_lua_init(lua_State* L, lj::Bson* config) {
        // register the configuration api.
        register_config_api(L, config);

        // Register the object model.
        Lunar<logjamd::Lua_record_set>::Register(L);
        Lunar<logjamd::Lua_storage>::Register(L);
        
        // Build the default storage object.
        push_default_storage(L, config);
        
        // Server ID.
        lua_pushinteger(L, rand());
        lua_setglobal(L, "server_id");
    }
    
    
    //=====================================================================
    // logjam global functions.
    //=====================================================================
    
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
