/*!
 \file Lua_storage.cpp
 \brief Logjamd lj::Storage wrapper for lua implementation.
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

#include "logjamd/Lua_storage.h"

#include "logjamd/Lua_bson.h"
#include "logjamd/Lua_record_set.h"
#include "logjamd/logjamd_lua.h"
#include "logjamd/lua_shared.h"
#include "logjamd/lua_shared.h"
#include "lj/Logger.h"
#include "lj/Storage_factory.h"
#include "lj/Time_tracker.h"
#include "build/default/config.h"

#include <string>
#include <sstream>

namespace logjamd
{
    const char Lua_storage::LUNAR_CLASS_NAME[] = "Storage";
    Lunar<Lua_storage>::RegType Lua_storage::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Lua_storage, all),
    LUNAR_MEMBER_METHOD(Lua_storage, none),
    LUNAR_MEMBER_METHOD(Lua_storage, at),
    LUNAR_MEMBER_METHOD(Lua_storage, place),
    LUNAR_MEMBER_METHOD(Lua_storage, remove),
    LUNAR_MEMBER_METHOD(Lua_storage, checkpoint),
    LUNAR_MEMBER_METHOD(Lua_storage, add_index),
    LUNAR_MEMBER_METHOD(Lua_storage, remove_index),
    LUNAR_MEMBER_METHOD(Lua_storage, rebuild),
    LUNAR_MEMBER_METHOD(Lua_storage, optimize),
    LUNAR_MEMBER_METHOD(Lua_storage, recall),
    {0, 0, 0}
    };
    
    Lua_storage::Lua_storage(const std::string& dbname) : dbname_(dbname)
    {
    }
    
    Lua_storage::Lua_storage(lua_State* L) : dbname_(lua_to_string(L, -1))
    {
    }
    
    Lua_storage::~Lua_storage()
    {
    }
    
    int Lua_storage::all(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        // Build the command.
        std::string cmd("db.");
        cmd.append(dbname_).append(":all()");
        
        // Create the record set.
        lj::Bson* cost_data = new lj::Bson();
        lj::Record_set* ptr = real_storage(L).all().release();
        Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
        Lunar<Lua_record_set>::push(L, wrapper, true);
        
        // Finish the debug info collection.
        timer.stop();
        cost_data->push_child("", lj::bson_new_cost(cmd,
                                                    timer.elapsed(),
                                                    ptr->raw_size(),
                                                    ptr->size()));        
        return 1;
    }
    
    int Lua_storage::none(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        // Build the command.
        std::string cmd("db.");
        cmd.append(dbname_).append(":none()");
        
        // Create the record set.
        lj::Bson* cost_data = new lj::Bson();
        lj::Record_set* ptr = real_storage(L).none().release();
        Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
        Lunar<Lua_record_set>::push(L, wrapper, true);
        
        // Finish the debug info collection.
        timer.stop();
        cost_data->push_child("", lj::bson_new_cost(cmd,
                                                    timer.elapsed(),
                                                    ptr->raw_size(),
                                                    ptr->size()));        
        return 1;
    }
    
    int Lua_storage::at(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        // Get the key to exclude.
        int key = luaL_checkint(L, -1);

        // Build the command executed.
        std::ostringstream cmd_builder;
        cmd_builder << "db." << dbname_;
        cmd_builder << ":at(" << key << ")";
        
        lj::Bson* cost_data = new lj::Bson();
        lj::Record_set* ptr = real_storage(L).at(key).release();
        Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
        Lunar<Lua_record_set>::push(L, wrapper, true);
        
        timer.stop();
        cost_data->push_child("", lj::bson_new_cost(cmd_builder.str(),
                                                    timer.elapsed(),
                                                    ptr->raw_size(),
                                                    ptr->size()));        
        return 1;
    }
    
    int Lua_storage::place(lua_State* L)
    {
        // {record}
        lj::Time_tracker timer;
        timer.start();

        // Get the configuration from the environment.
        logjamd::lua::sandbox_get(L, "lj__config"); // {record, config}
        const lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1); // {record}

        // Test the writable mode.
        if (logjamd::lua::is_mutable_write(config, __FUNCTION__))
        {
            // We can write, so lets execute the write logic.
            // starting with the pre-place event logic.
            get_event(L, dbname_, "pre_place"); // {record, event}
            if (!lua_isnil(L, -1))
            {
                lua_pushvalue(L, -2); // {record, event, record}
                lua_pushnil(L); // {record, event, record, nil}
                lua_call(L, 2, 1); // {record, bool}
            }
            else
            {
                lua_pop(L, 1); // {record}
                lua_pushboolean(L, true); // {record, bool}
            }
            
            // Test the event result.
            if (!lua_toboolean(L, -1))
            {
                lua_pop(L, 2); // {}
                lj::Log::debug.log("Not placing record.");
                timer.stop();
                // XXX this should update the costs array.
                return 0;
            }
            else
            {
                lua_pop(L, 1); // {record}
            }
            
            // Try to place the record.
            lj::Bson& record = Lunar<Lua_bson>::check(L, -1)->real_node();
            try
            {
                std::string server_id(lj::bson_as_string(config.nav("server/id")));
                lj::bson_increment(record.nav("__clock").nav(server_id), 1);
                record.set_child("__dirty", lj::bson_new_boolean(false));
                
                real_storage(L).place(record);
                
                const std::string replication_name(push_replication_record(L, record));
                push_replication_command(L, "place", dbname_, replication_name);
            }
            catch(lj::Exception* ex)
            {
                luaL_error(L, "Unable to place record. %s", ex->to_string().c_str());
            }
            
            // post place event logic.
            get_event(L, dbname_, "post_place"); // {record, event}
            if (!lua_isnil(L, -1))
            {
                lua_pushvalue(L, -2); // {record, event, record}
                lua_pushnil(L); // {record, event, record, nil}
                lua_call(L, 2, 0); // {record}
            }
            else
            {
                lua_pop(L, 1); // {record}
            }

            lua_pop(L, 1); // {}
        }
        else
        {
            // Not in a writable state. error out.
            luaL_error(L, "Unable to place record. Server is not in a writable mode.");
        }
        
        timer.stop();
        
        return 0;
    }
    
    int Lua_storage::remove(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        get_event(L, dbname_, "pre_remove");
        if (!lua_isnil(L, -1))
        {
            lua_pushvalue(L, -2);
            lua_pushnil(L);
            lua_call(L, 2, 1);
        }
        else
        {
            lua_pop(L, 1);
            lua_pushboolean(L, true);
        }
        
        if (!lua_toboolean(L, -1))
        {
            lj::Log::debug.log("Not removing record.");
            lua_pop(L, 2);
            return 0;
        }
        else
        {
            lua_pop(L, 1);
        }
        
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        
        try
        {
            lua_getglobal(L, "server_id");
            std::string server_id(lua_to_string(L, -1));
            lua_pop(L, 1);
            
            lj::bson_increment(ptr->real_node().nav("__clock").nav(server_id), 1);
            ptr->real_node().set_child("__dirty", lj::bson_new_boolean(false));
            real_storage(L).remove(ptr->real_node());
            
            const std::string replication_name(push_replication_record(L, ptr->real_node()));
            push_replication_command(L, "remove", dbname_, replication_name);
        }
        catch(lj::Exception* ex)
        {
            luaL_error(L, "Unable to remove record. %s", ex->to_string().c_str());
        }
        
        get_event(L, dbname_, "post_remove");
        if (!lua_isnil(L, -1))
        {
            lua_pushvalue(L, -2);
            lua_pushnil(L);
            lua_call(L, 2, 0);
        }
        else
        {
            lua_pop(L, 1);
        }        
        
        timer.stop();
        
        return 0;
    }
    
    int Lua_storage::checkpoint(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        real_storage(L).checkpoint();
        
        timer.stop();
        
        return 0;
    }
    
    int Lua_storage::add_index(lua_State* L)
    {
        // Get the data directory.
        lua_getglobal(L, "lj__config");
        const lj::Bson& server_config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        
        // Function args.
        std::string indxcomp(lua_to_string(L, -1));
        std::string indxfield(lua_to_string(L, -2));
        std::string indxtype(lua_to_string(L, -3));
        
        lj::Bson* cfg = real_storage(server_config).configuration();
        lj::storage_config_add_index(*cfg,
                                     indxtype,
                                     indxfield,
                                     indxcomp);
        lj::storage_config_save(*cfg, server_config);
        lj::Storage* ptr = lj::Storage_factory::reproduce(dbname_,
                                                          server_config);
        ptr->rebuild_field_index(indxfield);
        return 0;
    }
    
    int Lua_storage::remove_index(lua_State* L)
    {
        // Get the data directory.
        lua_getglobal(L, "lj__config");
        const lj::Bson& server_config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        
        // Function args
        std::string indxfield(lua_to_string(L, -1));
        std::string indxtype(lua_to_string(L, -2));
        
        lj::Bson* cfg = real_storage(server_config).configuration();
        lj::storage_config_remove_index(*cfg,
                                        indxtype,
                                        indxfield);
        lj::storage_config_save(*cfg, server_config);
        lj::Storage_factory::reproduce(dbname_, server_config);
        return 0;
    }
    
    int Lua_storage::rebuild(lua_State* L)
    {
        try
        {
            real_storage(L).rebuild();
        }
        catch (lj::Exception* ex)
        {
            std::string msg(ex->to_string());
            delete ex;
            return luaL_error(L, "Unable to rebuild indicies. %s", msg.c_str());
        }
        return 0;
    }
    
    int Lua_storage::optimize(lua_State* L)
    {
        try
        {
            real_storage(L).optimize();
        }
        catch (lj::Exception* ex)
        {
            std::string msg(ex->to_string());
            delete ex;
            return luaL_error(L, "Unable to optimize storage. %s", msg.c_str());
        }
        return 0;
    }
    
    int Lua_storage::recall(lua_State* L)
    {
        // Get the data directory.
        logjamd::lua::sandbox_get(L, "lj__config");
        lj::Bson& server_config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        
        lj::Storage_factory::recall(dbname_, server_config);
        return 0;
    }
    
    lj::Storage& Lua_storage::real_storage(lua_State* L)
    {
        // Get the data directory.
        logjamd::lua::sandbox_get(L, "lj__config");
        lj::Bson& server_config = Lunar<Lua_bson>::check(L, -1)->real_node();
        lua_pop(L, 1);
        
        return *(lj::Storage_factory::produce(dbname_, server_config));
    }
    
    lj::Storage& Lua_storage::real_storage(const lj::Bson& server_config)
    {
        return *(lj::Storage_factory::produce(dbname_, server_config));
    }
    
}; // namespace logjamd
