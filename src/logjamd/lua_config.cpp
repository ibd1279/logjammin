/*!
 \file lua_config.cpp
 \brief Logjam server lua functions for configuration.
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
#include "lj/Storage.h"
#include "lj/Storage_factory.h"
#include "build/default/config.h"

#include <string>

namespace
{
    //! Function buffer for reading and writing lua functions.
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
    
    //! Method for writing a function.
    int function_writer(lua_State*,
                        const void* p,
                        size_t sz,
                        void* ud)
    {
        Function_buffer* ptr = static_cast<Function_buffer*>(ud);
        return ptr->copy(p, sz);
    }
    
    //! Method for reading a function.
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
} // namespace (anonymous)

namespace logjamd
{
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

        int storage_event(lua_State* L)
        {
            // {storage, event, function}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            lj::Bson* function = 0;
            if (lua_isfunction(L, -1) && !lua_iscfunction(L, -1))
            {
                // dump the function to bson.
                Function_buffer buffer(10 * 1024);
                lua_dump(L, &function_writer, &buffer);
                function = lj::bson_new_binary(buffer.buf,
                                               buffer.cur - buffer.buf,
                                               lj::Bson::k_bin_function);
            }
            std::string event_name = lua_to_string(L, -2);
            std::string storage_name = lua_to_string(L, -3);
            lua_pop(L, 3);

            // create a pointer into the config for ease of reference.
            lj::Bson* storage_config = lj::storage_config_load(storage_name, config);

            // Construct the configuration path
            std::string handler_path("handler/");
            handler_path.append(event_name);

            // Sets the event handler to the value of function.
            // This depends on the behavior of set_child when
            // value is null (which is to remove the child).
            storage_config->set_child(handler_path, function);

            // Save the config file to disk.
            lj::storage_config_save(*storage_config, config);
            lj::Storage_factory::recall(storage_name, config);
            delete storage_config;
            return 0;
        }

        int storage_config(lua_State* L)
        {
            // {storage}
            lj::Bson& config = Lunar<logjamd::Lua_bson>::check(L, lua_upvalueindex(1))->real_node();
            std::string storage_name(lua_to_string(L, -1));
            lua_pop(L, 1); // {}
            
            lj::Bson* ptr = lj::storage_config_load(storage_name, config);
            Lunar<Lua_bson>::push(L, new Lua_bson(ptr, true), true);
            return 1;
        }
        
        void register_config_api(lua_State* L, lj::Bson* config)
        {
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
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &logjamd::lua::storage_event, 1); // {cfg, func}
            lua_setglobal(L, "lj_storage_event"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &logjamd::lua::storage_config, 1); // {cfg, func}
            lua_setglobal(L, "lj_storage_config"); // {cfg}

            lua_pop(L, 1); // {}
        }

        void load_autoload_storage(lua_State* L, const lj::Bson* config)
        {
            // Create the tables for storing the autoloads
            lua_newtable(L); // {db}
            int db_table = lua_gettop(L);
            lua_newtable(L); // {db, event}
            int event_table = lua_gettop(L);

            // Loop over the autoloaded storage names.
            const lj::Bson* default_storage = config->path("storage/autoload");
            if (!default_storage)
            {
                return;
            }

            for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter = default_storage->to_map().begin();
                 default_storage->to_map().end() != iter;
                 ++iter)
            {
                // load the storage and set it to the db table.
                std::string dbname(lj::bson_as_string(*iter->second));
                lua_pushstring(L, dbname.c_str()); // {db, event, dbname}
                logjamd::Lua_storage* db_ptr = new logjamd::Lua_storage(dbname);
                Lunar<logjamd::Lua_storage>::push(L, db_ptr, true); // {db, event, dbname, storage}
                lua_settable(L, db_table); // {db, event}
                
                // Loop over the events for the storage.
                lj::Bson* handlers = db_ptr->real_storage(*config).configuration()->path("handler");
                for (lj::Linked_map<std::string, lj::Bson*>::const_iterator iter2 = handlers->to_map().begin();
                     handlers->to_map().end() != iter2;
                     ++iter2)
                {
                    if (!iter2->second->exists())
                    {
                        lj::Log::debug.log("Skipping [%s] for [%s]")
                                << iter2->first
                                << dbname
                                << lj::Log::end;
                        continue;
                    }
                    
                    std::string event_name(dbname);
                    event_name.append("__").append(iter2->first);
                    lua_pushstring(L, event_name.c_str()); // {db, event, eventname}

                    Function_buffer state(iter2->second->size());

                    uint32_t sz = 0;
                    lj::Bson::Binary_type t = lj::Bson::k_bin_function;
                    const char* tmp = lj::bson_as_binary(*(iter2->second),
                                                         &t,
                                                         &sz);
                    state.copy(tmp, sz);
                    
                    if (lua_load(L, &function_reader, &state, event_name.c_str()))
                    {
                        // {db, event, eventname, error}
                        lj::Log::critical.log("Error loading function %s")
                                << lua_to_string(L, -1)
                                << lj::Log::end;
                        lua_pop(L, 2); // {db, event}
                    }
                    else
                    {
                        // {db, event, eventname, function}
                        lua_settable(L, event_table); // {db, event}
                    }
                }
            }
            lua_setglobal(L, "db_events"); // {db}
            lua_setglobal(L, "db"); // {}
        }

    }; // namespace logjamd::lua

}; // namespace logjamd
