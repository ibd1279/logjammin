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
    LUNAR_MEMBER_METHOD(Lua_storage, rebuild),
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
        cmd.append(real_storage().name()).append(":all()");
        
        // Create the record set.
        lj::Bson* cost_data = new lj::Bson();
        lj::Record_set* ptr = real_storage().all().release();
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
        cmd.append(real_storage().name()).append(":none()");
        
        // Create the record set.
        lj::Bson* cost_data = new lj::Bson();
        lj::Record_set* ptr = real_storage().none().release();
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
        cmd_builder << "include(" << key << ")";
        
        lj::Bson* cost_data = new lj::Bson();
        lj::Record_set* ptr = real_storage().at(key).release();
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
        lj::Time_tracker timer;
        timer.start();
        
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        try
        {
            real_storage().place(ptr->real_node());
        }
        catch(lj::Exception* ex)
        {
            luaL_error(L, "Unable to place record. %s", ex->to_string().c_str());
        }
        
        timer.stop();
        
        return 0;
    }
    
    int Lua_storage::remove(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        real_storage().remove(ptr->real_node());
        
        timer.stop();
        
        return 0;
    }
    
    int Lua_storage::checkpoint(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        real_storage().checkpoint();
        
        timer.stop();
        
        return 0;
    }
    
    int Lua_storage::add_index(lua_State* L)
    {
        std::string indxcomp(lua_to_string(L, -1));
        std::string indxfield(lua_to_string(L, -2));
        std::string indxtype(lua_to_string(L, -3));
        
        lj::Bson* cfg = real_storage().configuration();
        lj::storage_config_add_index(*cfg,
                                     indxtype,
                                     indxfield,
                                     indxcomp);
        lj::storage_config_save(*cfg);
        
        std::string storage_name = real_storage().name();
        lj::Storage* ptr = lj::Storage_factory::reproduce(storage_name);
        ptr->rebuild();
        return 0;
    }
    
    int Lua_storage::rebuild(lua_State* L)
    {
        real_storage().rebuild();
        return 0;
    }
    
    lj::Storage& Lua_storage::real_storage()
    {
        return *(lj::Storage_factory::produce(dbname_));
    }
    
}; // namespace logjamd
