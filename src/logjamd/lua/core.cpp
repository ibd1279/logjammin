/*!
 \file logjamd/lua/core.cpp
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

#include "logjamd/lua/core.h"

#include "logjamd/lua/Bson.h"
#include "logjamd/lua/Storage.h"
#include "logjamd/lua/Record_set.h"
#include "logjamd/logjamd_lua.h"
#include "lj/Logger.h"
#include "lj/Storage.h"
#include "lj/Storage_factory.h"
#include "build/default/config.h"

#include <string>

namespace
{
    const std::string k_delayed_effect_log_message("[%s] changed to [%s]. Change will not apply until the server is restarted.");

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
    }; // struct (anonymous)::Function_buffer
    
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

    //! Utility method for persisting configuration.
    void util_persist_config(lua_State* L, const lj::Bson& config)
    {
        // Disk save first, incase of failure.
        const lj::Bson& configfile = config.nav("server/configfile");
        lj::bson_save(config, lj::bson_as_string(configfile));

        // environment next
        logjamd::lua::Bson* wrapped_config = new logjamd::lua::Bson(new lj::Bson(config), true);
        logjamd::lua::sandbox_push(L); // {env}
        Lunar<logjamd::lua::Bson>::push(L, wrapped_config, true); // {env, cfg}
        lua_setfield(L, -2, "lj__config"); // {env}
        lua_pop(L, 1); // {}
    }

    int server_port(lua_State* L)
    {
        // {arg}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        int port = lua_tointeger(L, -1);
        lua_pop(L, 1); // {}

        // Test that config commands are currently allowed.
        if (!logjamd::is_mutable_config(config, __FUNCTION__))
        {
            return 0;
        }

        // Set the new value.
        config.set_child("server/port", lj::bson_new_int64(port));

        // Save the config to disk, and update env.
        util_persist_config(L, config);

        // Write a log entry for the config change.
        lj::Log::alert.log(k_delayed_effect_log_message)
                << "server/port"
                << port
                << lj::Log::end;
        return 0;
    }

    int server_directory(lua_State* L)
    {
        // {arg}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string directory = lua_to_string(L, -1);
        lua_pop(L, 1); // {}

        // Test that config commands are currently allowed.
        if (!logjamd::is_mutable_config(config, __FUNCTION__))
        {
            return 0;
        }

        // Set the new value.
        config.set_child("server/directory", lj::bson_new_string(directory));

        // Save the config file to disk.
        util_persist_config(L, config);

        // Write a log entry for the config change.
        lj::Log::alert.log(k_delayed_effect_log_message)
                << "server/directory"
                << directory
                << lj::Log::end;
        return 0;
    }

    int server_id(lua_State* L)
    {
        // {arg}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string server_id = lua_to_string(L, -1);
        lua_pop(L, 1); // {}

        // Test that config commands are currently allowed.
        if (!logjamd::is_mutable_config(config, __FUNCTION__))
        {
            return 0;
        }

        // Set the new value.
        config.set_child("server/id", lj::bson_new_string(server_id));

        // Save the config file to disk.
        util_persist_config(L, config);

        // Write a log entry for the config change.
        lj::Log::alert.log(k_delayed_effect_log_message)
                << "server/id"
                << server_id
                << lj::Log::end;
        return 0;
    }

    int storage_autoload(lua_State* L)
    {
        // {cmd, storage}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string storage = lua_to_string(L, -1);
        std::string command = lua_to_string(L, -2);
        lua_pop(L, 2); // {}

        // Test that config commands are currently allowed.
        if (!logjamd::is_mutable_config(config, __FUNCTION__))
        {
            return 0;
        }

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
        command.push_back(' ');
        command.append(storage);
        lj::Log::alert.log(k_delayed_effect_log_message)
                << "storage/autoload"
                << command
                << lj::Log::end;
        return 0;
    }

    int replication_peer(lua_State* L)
    {
        // {cmd, peer}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string peer = lua_to_string(L, -1);
        std::string command = lua_to_string(L, -2);
        lua_pop(L, 2); // {}

        // Test that config commands are currently allowed.
        if (!logjamd::is_mutable_config(config, __FUNCTION__))
        {
            return 0;
        }

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
        command.push_back(' ');
        command.append(peer);
        lj::Log::alert.log(k_delayed_effect_log_message)
                << "replication/peer"
                << command
                << lj::Log::end;
        return 0;
    }

    int logging_level(lua_State* L)
    {
        // {level, enabled}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        bool enabled = lua_toboolean(L, -1);
        std::string level = lua_to_string(L, -2);
        lua_pop(L, 2); // {}

        // set the value.
        config.nav("logging").set_child(level, lj::bson_new_boolean(enabled));

        // Logging level can be changed even when not in a configurable state.
        // the difference is that the configuration change will not be saved
        // unless we are in a configurable mode.
        if (logjamd::is_mutable_config(config, __FUNCTION__))
        {
            // Save the config file to disk.
            util_persist_config(L, config);
        }

        // modify the current logging levels.
        logjamd::set_logging_levels(config);

        // Write a log entry for the config change.
        lj::Log::alert.log("[logging/%s] changed to [%s].")
                << level
                << enabled
                << lj::Log::end;
        return 0;
    }

    int storage_init(lua_State* L)
    {
        // {name}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string storage_name = lua_to_string(L, -1);
        lua_pop(L, 1); // {}

        // Test that write commands are currently allowed.
        if (!logjamd::is_mutable_write(config, __FUNCTION__))
        {
            return 0;
        }

        lj::Bson storage_config;
        lj::storage_config_init(storage_config, storage_name);
        lj::storage_config_save(storage_config, config);
        lj::Storage_factory::recall(storage_name, config);

        return 0;
    }

    int storage_index(lua_State* L)
    {
        // {storage, field, type, compare}
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string index_comparison(lua_to_string(L, -1));
        std::string index_type(lua_to_string(L, -2));
        std::string index_field(lua_to_string(L, -3));
        std::string storage_name(lua_to_string(L, -4));
        lua_pop(L, 4); // {}

        // Test that write commands are currently allowed.
        if (!logjamd::is_mutable_write(config, __FUNCTION__))
        {
            return 0;
        }

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
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string field(lua_to_string(L, -1));
        std::string storage_name(lua_to_string(L, -2));
        lua_pop(L, 2); // {}

        // Test that write commands are currently allowed.
        if (!logjamd::is_mutable_write(config, __FUNCTION__))
        {
            return 0;
        }

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
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
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

        // Test that write commands are currently allowed.
        if (!logjamd::is_mutable_write(config, __FUNCTION__))
        {
            return 0;
        }

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
        lj::Bson& config = Lunar<logjamd::lua::Bson>::check(L, lua_upvalueindex(1))->real_node();
        std::string storage_name(lua_to_string(L, -1));
        lua_pop(L, 1); // {}
        
        // Test that write commands are currently allowed.
        if (!logjamd::is_mutable_read(config, __FUNCTION__))
        {
            return 0;
        }

        lj::Bson* ptr = lj::storage_config_load(storage_name, config);
        Lunar<logjamd::lua::Bson>::push(L, new logjamd::lua::Bson(ptr, true), true);
        return 1;
    }

    int send_item(lua_State* L)
    {        
        // {item}
        logjamd::lua::sandbox_get(L, "lj__response"); // {item, response}
        logjamd::lua::Bson* response = Lunar<logjamd::lua::Bson>::check(L, -1);
        logjamd::lua::Bson* item = Lunar<logjamd::lua::Bson>::check(L, -2);
        lua_pop(L, 2); // {}
        response->real_node().push_child("item", new lj::Bson(item->real_node()));
        return 0;
    }

    int print(lua_State* L)
    {
        // {item}
        logjamd::lua::sandbox_get(L, "lj__response"); // {arg, response}
        logjamd::lua::Bson* response = Lunar<logjamd::lua::Bson>::check(L, -1);
        std::string arg = lua_to_string(L, -2);
        lua_pop(L, 2); // {}
        response->real_node().push_child("lj__output", lj::bson_new_string(arg));
        return 0;
    }

    int send_set(lua_State *L)
    {
        // {record_set}
        lj::Time_tracker timer;

        // Get what we need from lua's stack
        logjamd::lua::Record_set* filter = Lunar<logjamd::lua::Record_set>::check(L, -1);
        lua_pop(L, 1); // {}
        
        const std::string k_command(logjamd::lua::command_from_costs("send_set(",
                                                                     ")",
                                                                     filter->costs()));
        
        // copy the costs, incase they use the result set more than once.
        lj::Bson* cost_data = new lj::Bson(filter->costs());
        
        // Get the items for the result set.
        lj::Bson* items = new lj::Bson();
        filter->real_set().items_raw(*items);
        
        // Push the result.
        logjamd::lua::result_push(L,
                                  k_command,
                                  "send_set",
                                  cost_data,
                                  items,
                                  timer);
        
        return 0;
    }

    int help(lua_State* L)
    {
        logjamd::lua::sandbox_get(L, "lj__response"); // {arg, response}
        lj::Bson& response = Lunar<logjamd::lua::Bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        response["lj__help/common"]
                << lj::bson_new_string("send_item(bson)")
                << lj::bson_new_string("print(string)")
                << lj::bson_new_string("send_set(record_set)")
                << lj::bson_new_string("help()")
                << lj::bson_new_string("Bson:nav(path)")
                << lj::bson_new_string("Bson:set(value)")
                << lj::bson_new_string("Bson:push(value)")
                << lj::bson_new_string("Bson:get()")
                << lj::bson_new_string("Bson:save(filename)")
                << lj::bson_new_string("Bson:load(filename)")
                << lj::bson_new_string("Bson.<path>")
                << lj::bson_new_string("Storage:none()")
                << lj::bson_new_string("Storage:all()")
                << lj::bson_new_string("Storage:place(record)")
                << lj::bson_new_string("Storage:remove(record)")
                << lj::bson_new_string("Storage:at(id)")
                << lj::bson_new_string("Storage:rebuild()")
                << lj::bson_new_string("Storage:checkpoint()")
                << lj::bson_new_string("Storage:optimize()")
                << lj::bson_new_string("Storage:recall()")
                << lj::bson_new_string("Record_set:mode_and()")
                << lj::bson_new_string("Record_set:mode_or()")
                << lj::bson_new_string("Record_set:include(id)")
                << lj::bson_new_string("Record_set:include(function (b) if include then return true else return false end end)")
                << lj::bson_new_string("Record_set:exclude(id)")
                << lj::bson_new_string("Record_set:exclude(function (b) if exclude then return true else return false end end)")
                << lj::bson_new_string("Record_set:equal(field, value)")
                << lj::bson_new_string("Record_set:greater(field, value)")
                << lj::bson_new_string("Record_set:lesser(field, value)")
                << lj::bson_new_string("Record_set:contains(field, value)")
                << lj::bson_new_string("Record_set:tagged(field, value)")
                << lj::bson_new_string("Record_set:records()")
                << lj::bson_new_string("Record_set:first()")
                << lj::bson_new_string("Record_set:size()");

        response["lj__help/server"]
                << lj::bson_new_string("lj__server_port(port)")
                << lj::bson_new_string("lj__server_directory(directory)")
                << lj::bson_new_string("lj__server_id(id)")
                << lj::bson_new_string("lj__storage_autoload('add', name)")
                << lj::bson_new_string("lj__storage_autoload('rm', name)")
                << lj::bson_new_string("lj__replication_peer('add', peer)")
                << lj::bson_new_string("lj__replication_peer('rm', peer)")
                << lj::bson_new_string("lj__logging_level(level, enable)")
                << lj::bson_new_string("lj_storage_init(name)")
                << lj::bson_new_string("lj_storage_index(name, field, type, compare)")
                << lj::bson_new_string("lj_storage_subfield(name, field)")
                << lj::bson_new_string("lj_storage_event(name, event, function)")
                << lj::bson_new_string("lj_storage_config(name)");

        return 0;

    }

} // namespace (anonymous)

namespace logjamd
{
    bool check_mutable_mode(const lj::Bson& config, const Mutable_mode mode)
    {
        const lj::Bson& tmp = config.nav("server/mode");
        return (mode == static_cast<Mutable_mode>(lj::bson_as_int64(tmp)));
    }

    namespace lua
    {
        void register_config_api(lua_State* L, lj::Bson* config)
        {
            // Register the minimum required functions.
            lua_register(L, "send_item", &send_item);
            lua_register(L, "print", &print);
            lua_register(L, "send_set", &send_set);
            lua_register(L, "help", &help);

            // Push the configuration onto the stack for closures.
            Lunar<logjamd::lua::Bson>::push(L, new logjamd::lua::Bson(config, false), true); // {cfg}

            // load the server configuration functions.
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &server_port, 1); // {cfg, func}
            lua_setglobal(L, "lj__server_port"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &server_directory, 1); // {cfg, func}
            lua_setglobal(L, "lj__server_directory"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &server_id, 1); // {cfg, func}
            lua_setglobal(L, "lj__server_id"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &storage_autoload, 1); // {cfg, func}
            lua_setglobal(L, "lj__storage_autoload"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &replication_peer, 1); // {cfg, func}
            lua_setglobal(L, "lj__replication_peer"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &logging_level, 1); // {cfg, func}
            lua_setglobal(L, "lj__logging_level"); // {cfg}

            // load the storage configuration functions.
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &storage_init, 1); // {cfg, func}
            lua_setglobal(L, "lj_storage_init"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &storage_index, 1); // {cfg, func}
            lua_setglobal(L, "lj_storage_index"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &storage_subfield, 1); // {cfg, func}
            lua_setglobal(L, "lj_storage_subfield"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &storage_event, 1); // {cfg, func}
            lua_setglobal(L, "lj_storage_event"); // {cfg}
            lua_pushvalue(L, -1); // {cfg, cfg}
            lua_pushcclosure(L, &storage_config, 1); // {cfg, func}
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
                logjamd::lua::Storage* db_ptr = new logjamd::lua::Storage(dbname);
                Lunar<logjamd::lua::Storage>::push(L, db_ptr, true); // {db, event, dbname, storage}
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

        std::string command_from_costs(const std::string& prefix,
                                       const std::string& suffix,
                                       const lj::Bson& costs)
        {
            std::string cmd(prefix);
            for (auto iter = costs.to_map().begin();
                 costs.to_map().end() != iter;
                 ++iter)
            {
                cmd.append(lj::bson_as_string(*(*iter).second->path("cmd")));
                cmd.append(":");
            }
            cmd.erase(cmd.size() - 1).append(suffix);
            return cmd;
        }

        int result_push(lua_State* L,
                        const std::string& full_cmd,
                        const std::string& current_cmd,
                        lj::Bson* cost_data,
                        lj::Bson* items,
                        lj::Time_tracker& timer)
        {
            // {}
            logjamd::lua::sandbox_get(L, "lj__response"); // {response}
            lj::Bson& response = Lunar<lua::Bson>::check(L, -1)->real_node();
            lua_pop(L, 1); // {}

            // Normalize cost and items data.
            if (!cost_data)
            {
                cost_data = new lj::Bson();
            }

            if (!items)
            {
                items = new lj::Bson();
            }

            // build the result.
            auto item_size = items->to_map().size();
            lj::Bson* result = new lj::Bson();
            result->set_child("cmd", lj::bson_new_string(full_cmd));
            result->set_child("costs", cost_data);
            if (item_size > 0)
            {
                result->set_child("items", items);
            }

            // add the result to the response.
            response.push_child("results", result);

            // Add the last cost to the result.
            cost_data->push_child("", lj::bson_new_cost(current_cmd,
                                                        timer.elapsed(),
                                                        item_size,
                                                        item_size));

            return 0;
        }

        const lj::Bson& get_configuration(lua_State* L)
        {
            // {}
            logjamd::lua::sandbox_get(L, "lj__config"); // {config}
            const lj::Bson& config = Lunar<lua::Bson>::check(L, -1)->real_node();
            lua_pop(L, 1); // {}
            return config;
        }

        int fail(lua_State* L,
                 const std::string& command,
                 const std::string& msg,
                 lj::Time_tracker& timer)
        {
            logjamd::lua::result_push(L,
                                      command,
                                      command, 
                                      NULL,
                                      NULL,
                                      timer);

            std::string fmt(command + " failed. [%s]");
            return luaL_error(L,
                              fmt.c_str(),
                              msg.c_str());

        }
    }; // namespace logjamd::lua

}; // namespace logjamd
