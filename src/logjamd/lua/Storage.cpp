/*!
 \file Storage.cpp
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

#include "logjamd/lua/Storage.h"

#include "logjamd/lua/core.h"
#include "logjamd/Lua_bson.h"
#include "logjamd/Lua_record_set.h"
#include "logjamd/logjamd_lua.h"
#include "lj/Logger.h"
#include "lj/Storage_factory.h"
#include "lj/Time_tracker.h"
#include "build/default/config.h"

#include <string>
#include <sstream>

namespace logjamd
{
    namespace lua
    {
        const char Storage::LUNAR_CLASS_NAME[] = "Storage";
        Lunar<Storage>::RegType Storage::LUNAR_METHODS[] = {
        LUNAR_MEMBER_METHOD(Storage, all),
        LUNAR_MEMBER_METHOD(Storage, none),
        LUNAR_MEMBER_METHOD(Storage, at),
        LUNAR_MEMBER_METHOD(Storage, place),
        LUNAR_MEMBER_METHOD(Storage, remove),
        LUNAR_MEMBER_METHOD(Storage, checkpoint),
        LUNAR_MEMBER_METHOD(Storage, rebuild),
        LUNAR_MEMBER_METHOD(Storage, optimize),
        LUNAR_MEMBER_METHOD(Storage, recall),
        {0, 0, 0}
        };
        
        Storage::Storage(const std::string& dbname) : dbname_(dbname)
        {
        }
        
        Storage::Storage(lua_State* L) : dbname_(lua_to_string(L, -1))
        {
        }
        
        Storage::~Storage()
        {
        }
        
        int Storage::all(lua_State* L)
        {
            lj::Time_tracker timer;
            
            // Build the command.
            const std::string k_command(std::string("db.") +
                                        dbname_ +
                                        ":all()");
            
            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the readable mode.
            if (!logjamd::is_mutable_read(config, __FUNCTION__))
            {
                // Not in a readable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a read mode.",
                                          timer);
            }

            // Create the record set.
            lj::Bson* cost_data = new lj::Bson();
            lj::Record_set* ptr = real_storage(L).all().release();
            Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
            Lunar<Lua_record_set>::push(L, wrapper, true);
            
            // Finish the cost info collection.
            cost_data->push_child("", lj::bson_new_cost(k_command,
                                                        timer.elapsed(),
                                                        ptr->raw_size(),
                                                        ptr->size()));        
            return 1;
        }
        
        int Storage::none(lua_State* L)
        {
            lj::Time_tracker timer;
            
            // Build the command.
            const std::string k_command(std::string("db.") +
                                        dbname_ +
                                        ":none()");

            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the readable mode.
            if (!logjamd::is_mutable_read(config, __FUNCTION__))
            {
                // Not in a readable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a read mode.",
                                          timer);
            }

            // Create the record set.
            lj::Bson* cost_data = new lj::Bson();
            lj::Record_set* ptr = real_storage(L).none().release();
            Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
            Lunar<Lua_record_set>::push(L, wrapper, true);
            
            // Finish the cost info collection.
            cost_data->push_child("", lj::bson_new_cost(k_command,
                                                        timer.elapsed(),
                                                        ptr->raw_size(),
                                                        ptr->size()));        
            return 1;
        }
        
        int Storage::at(lua_State* L)
        {
            lj::Time_tracker timer;
            
            // Get the key to exclude.
            int key = luaL_checkint(L, -1);

            // Build the command executed.
            std::ostringstream cmd_builder;
            cmd_builder << "db." << dbname_;
            cmd_builder << ":at(" << key << ")";
            const std::string k_command(cmd_builder.str());
            
            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the readable mode.
            if (!logjamd::is_mutable_read(config, __FUNCTION__))
            {
                // Not in a readable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a read mode.",
                                          timer);
            }

            lj::Bson* cost_data = new lj::Bson();
            lj::Record_set* ptr = real_storage(L).at(key).release();
            Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
            Lunar<Lua_record_set>::push(L, wrapper, true);
        
            // Finish the cost info collection.
            cost_data->push_child("", lj::bson_new_cost(k_command,
                                                        timer.elapsed(),
                                                        ptr->raw_size(),
                                                        ptr->size()));        
            return 1;
        }
        
        int Storage::place(lua_State* L)
        {
            // {record}
            lj::Time_tracker timer;

            // Create the command name.
            const std::string k_command(std::string("db.") +
                                        dbname_ +
                                        ".place(<record>)");

            // validate the input before we begin.
            Lua_bson* wrapped_record = Lunar<Lua_bson>::check(L, -1);

            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the writable mode.
            if (!logjamd::is_mutable_write(config, __FUNCTION__))
            {
                // Not in a writable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a write mode.",
                                          timer);
            }

            // We can write, so lets execute the write logic.
            lj::Log::info("Place record in storage [%s].",
                          dbname_.c_str());

            // Start by protecting the original object.
            lj::Bson& original_record = wrapped_record->real_node();
            lj::Bson record(original_record);

            // Invoke the pre-placement event.
            get_event(L, dbname_, "pre_place"); // {record, event}
            if (!lua_isnil(L, -1))
            {
                lj::Log::debug(".. Found pre-placement event. Executing.");
                lua_pushvalue(L, -2); // {record, event, record}
                lua_pushnil(L); // {record, event, record, nil}
                lua_call(L, 2, 1); // {record, bool}
            }
            else
            {
                lj::Log::debug(".. No pre-placement event found.");
                lua_pop(L, 1); // {record}
                lua_pushboolean(L, true); // {record, bool}
            }
            
            // Test the event result.
            if (!lua_toboolean(L, -1))
            {
                lua_pop(L, 2); // {}
                lj::Log::debug(".. Pre-palcement returned false. Not placing record.");
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Pre-placement returned false.",
                                          timer);
            }
            else
            {
                lua_pop(L, 1); // {record}
                lj::Log::debug(".. Finished pre-palcement events. continuing.");
            }
            
            // Try to place the record.
            try
            {
                // Modify internal structures on the object prior to placing.
                lj::Log::debug(".. preparing record.");
                std::string server_id(lj::bson_as_string(config.nav("server/id")));
                lj::bson_increment(record.nav("__clock").nav(server_id), 1);
                record.set_child("__dirty", lj::bson_new_boolean(false));

                lj::Log::debug(".. executing placement.");
                real_storage(L).place(record);
                
                lj::Log::debug(".. recording replication information.");
                // XXX Replication functionality.

                lj::Log::debug(".. placement complete.");
            }
            catch(lj::Exception* ex)
            {
                // Clean things up.
                std::string msg(ex->to_string());
                delete ex;
                lua_pop(L, 1); // {}

                // Log the exception.
                lj::Log::info.log("Unable to place record in [%s]. [%s]")
                        << dbname_
                        << msg
                        << lj::Log::end;

                lj::Log::debug(".. erasing replication information.");
                // XXX need some rollback logic on the replication stuff.

                return logjamd::lua::fail(L,
                                          k_command,
                                          msg,
                                          timer);
            }
            
            lj::Log::debug(".. updating record.");
            original_record.copy_from(record);

            // Post placement event logic.
            get_event(L, dbname_, "post_place"); // {record, event}
            if (!lua_isnil(L, -1))
            {
                lj::Log::debug(".. Found post-placement event. Executing.");
                lua_pushvalue(L, -2); // {record, event, record}
                lua_pushnil(L); // {record, event, record, nil}
                lua_call(L, 2, 0); // {record}
            }
            else
            {
                lj::Log::debug(".. No post-placement event found.");
                lua_pop(L, 1); // {record}
            }

            lua_pop(L, 1); // {}

            lj::Log::info("Completed place record in storage [%s].",
                          dbname_.c_str());
            logjamd::lua::result_push(L,
                                      k_command,
                                      k_command, 
                                      NULL,
                                      NULL,
                                      timer);

            return 0;
        }
        
        int Storage::remove(lua_State* L)
        {
            // {record}
            lj::Time_tracker timer;

            // Create the command name.
            const std::string k_command(std::string("db.") +
                                        dbname_ +
                                        ".remove(<record>)");
            
            // Validate the input before we begin.
            Lua_bson* wrapped_record = Lunar<Lua_bson>::check(L, -1);

            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the writable mode.
            if (!logjamd::is_mutable_write(config, __FUNCTION__))
            {
                // Not in a writable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a write mode.",
                                          timer);
            }

            // We can write, so lets execute the remove logic.
            lj::Log::info("Remove record in storage [%s].",
                          dbname_.c_str());

            // Invoke the pre-removal event.
            get_event(L, dbname_, "pre_remove"); // {record, event}
            if (!lua_isnil(L, -1))
            {
                lj::Log::debug(".. Found pre-removal event. Executing.");
                lua_pushvalue(L, -2); // {record, event, record}
                lua_pushnil(L); // {record, event, record, nil}
                lua_call(L, 2, 1); // {record, bool}
            }
            else
            {
                lj::Log::debug(".. No pre-removal event found.");
                lua_pop(L, 1); // {record}
                lua_pushboolean(L, true); // {record, bool}
            }
            
            // Test the event result.
            if (!lua_toboolean(L, -1))
            {
                lua_pop(L, 2); // {}
                lj::Log::debug(".. Pre-removal returned false. Not removing record.");
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Pre-removal returned false.",
                                          timer);
            }
            else
            {
                lua_pop(L, 1); // {record}
                lj::Log::debug(".. Finished pre-removal events. continuing.");
            }
            
            // Un-wrap the argument.
            lj::Bson& record = wrapped_record->real_node();

            // try to remove the record.
            try
            {
                lj::Log::debug(".. executing removal.");
                real_storage(L).remove(record);

                lj::Log::debug(".. recording replication information.");
                // XXX Replication functionality.

                lj::Log::debug(".. removal complete.");
            }
            catch(lj::Exception* ex)
            {
                // Clean things up.
                std::string msg(ex->to_string());
                delete ex;
                lua_pop(L, 1); // {}

                lj::Log::info.log("Unable to remove record from [%s]. [%s].")
                        << dbname_
                        << msg
                        << lj::Log::end;
                
                lj::Log::debug(".. erasing replication information.");
                // XXX need some rollback logic on the replication stuff.

                return logjamd::lua::fail(L,
                                          k_command,
                                          msg,
                                          timer);
            }
            
            // Post removal event logic.
            get_event(L, dbname_, "post_remove");
            if (!lua_isnil(L, -1))
            {
                lj::Log::debug(".. Found post-removal event. Executing.");
                lua_pushvalue(L, -2); // {record, event, record}
                lua_pushnil(L); // {record, event, record, nil}
                lua_call(L, 2, 0); // {record}
            }
            else
            {
                lj::Log::debug(".. No post-removal event found.");
                lua_pop(L, 1); // {record}
            }

            lua_pop(L, 1); // {}

            lj::Log::info("Completed remove record from storage [%s].",
                          dbname_.c_str());
            logjamd::lua::result_push(L,
                                      k_command,
                                      k_command, 
                                      NULL,
                                      NULL,
                                      timer);
            
            return 0;
        }
        
        int Storage::checkpoint(lua_State* L)
        {
            lj::Time_tracker timer;

            const std::string k_command(std::string("db.") +
                                        dbname_ +
                                        ":checkpoint()");
            
            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the writable mode.
            if (!logjamd::is_mutable_write(config, __FUNCTION__))
            {
                // Not in a writable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a write mode.",
                                          timer);
            }

            try
            {
                lj::Log::info("Performing checkpoint on [%s].",
                              dbname_.c_str());
                real_storage(L).checkpoint();
            }
            catch (lj::Exception* ex)
            {
                std::string msg(ex->to_string());
                delete ex;
                return logjamd::lua::fail(L,
                                          k_command,
                                          msg,
                                          timer);
            }

            lj::Log::info("Completed checkpoint on [%s].",
                          dbname_.c_str());
            logjamd::lua::result_push(L,
                                      k_command,
                                      k_command, 
                                      NULL,
                                      NULL,
                                      timer);
            
            return 0;
        }
        
        int Storage::rebuild(lua_State* L)
        {
            lj::Time_tracker timer;

            const std::string k_command(std::string("db.") +
                                        dbname_ +
                                        ":rebuild()");

            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the writable mode.
            if (!logjamd::is_mutable_write(config, __FUNCTION__))
            {
                // Not in a writable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a write mode.",
                                          timer);
            }

            try
            {
                lj::Log::info("Performing rebuild on [%s].",
                              dbname_.c_str());
                real_storage(L).rebuild();
            }
            catch (lj::Exception* ex)
            {
                std::string msg(ex->to_string());
                delete ex;
                return logjamd::lua::fail(L,
                                          k_command,
                                          msg,
                                          timer);
            }

            lj::Log::info("Completed rebuild on [%s].",
                          dbname_.c_str());
            logjamd::lua::result_push(L,
                                      k_command,
                                      k_command, 
                                      NULL,
                                      NULL,
                                      timer);
            
            return 0;
        }
        
        int Storage::optimize(lua_State* L)
        {
            lj::Time_tracker timer;

            const std::string k_command(std::string("db.") +
                                        dbname_ +
                                        ":optimize()");

            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);

            // Test the writable mode.
            if (!logjamd::is_mutable_write(config, __FUNCTION__))
            {
                // Not in a writable state. error out.
                return logjamd::lua::fail(L,
                                          k_command,
                                          "Server is not in a write mode.",
                                          timer);
            }

            try
            {
                lj::Log::info("Performing optimize on [%s].",
                              dbname_.c_str());
                real_storage(L).optimize();
            }
            catch (lj::Exception* ex)
            {
                std::string msg(ex->to_string());
                delete ex;
                return logjamd::lua::fail(L,
                                          k_command,
                                          msg,
                                          timer);
            }
            lj::Log::info("Completed rebuild on [%s].",
                          dbname_.c_str());
            logjamd::lua::result_push(L,
                                      k_command,
                                      k_command, 
                                      NULL,
                                      NULL,
                                      timer);
            
            return 0;
        }
        
        int Storage::recall(lua_State* L)
        {
            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);
            
            lj::Storage_factory::recall(dbname_, config);
            return 0;
        }
        
        lj::Storage& Storage::real_storage(lua_State* L)
        {
            // Get the configuration from the environment.
            const lj::Bson& config = logjamd::lua::get_configuration(L);
            
            return *(lj::Storage_factory::produce(dbname_, config));
        }
        
        lj::Storage& Storage::real_storage(const lj::Bson& config)
        {
            return *(lj::Storage_factory::produce(dbname_, config));
        }
    }; // namespace logjamd::lua
}; // namespace logjamd
