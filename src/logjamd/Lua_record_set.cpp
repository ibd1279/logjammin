/*!
 \file Lua_record_set.cpp
 \brief Logjamd lj::Record_set wrapper implement.
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

#include "Lua_record_set.h"

#include "logjamd/lua/core.h"
#include "logjamd/lua/Storage.h"
#include "logjamd/Lua_bson.h"
#include "lj/Bson.h"
#include "lj/Logger.h"
#include "lj/Standard_record_set.h"
#include "lj/Storage.h"
#include "lj/Time_tracker.h"

#include <string>
#include <memory>
#include <sstream>

using namespace logjamd::lua;

namespace
{
    void text_filter(lua_State* L,
                     lj::Record_set& real_set,
                     std::unique_ptr<lj::Record_set> (lj::Record_set::*f)(const std::string&, const std::string&) const,
                     const std::string& cmd,
                     const lj::Bson& costs)
    {
        lj::Time_tracker timer;

        // copy cost data for the new result.
        lj::Bson* cost_data = new lj::Bson(costs);
        
        // Get the search inputs
        std::string field(lua_to_string(L, -2));
        std::string val(lua_to_string(L, -1));
        
        // Start collecting some cost information.
        std::ostringstream cmd_builder;
        cmd_builder << cmd << "(";
        cmd_builder << "'" << field << "', '" << val << "')";
        const std::string k_command(cmd_builder.str());
        
        // Execute the filtering operation.
        lj::Record_set* ptr = (real_set.*f)(field, val).release();
        logjamd::Lua_record_set* wrapper = new logjamd::Lua_record_set(ptr, cost_data);
        Lunar<logjamd::Lua_record_set>::push(L, wrapper, true);
        
        cost_data->push_child("", lj::bson_new_cost(k_command,
                                                    timer.elapsed(),
                                                    ptr->raw_size(),
                                                    ptr->size()));
    }
    void filter(lua_State* L, 
                lj::Record_set& real_set,
                std::unique_ptr<lj::Record_set> (lj::Record_set::*f)(const std::string&, const void* const, const size_t) const,
                const std::string& cmd,
                const lj::Bson& costs)
    {
        lj::Time_tracker timer;

        // copy cost data for the new result.
        lj::Bson* cost_data = new lj::Bson(costs);
        
        // Get the field from the lua stack.
        std::string field(lua_to_string(L, -2));
        
        // Start collecting some cost information.
        std::ostringstream cmd_builder;
        cmd_builder << cmd << "(";
        cmd_builder << "'" << field << "', ";
        
        // Execute the filtering operation.
        lj::Record_set* ptr;
        lj::Log::info.log("doing a compare with is_string %d is_number %d")
                << lua_isstring(L, -1)
                << lua_isnumber(L, -1)
                << lj::Log::end;

        if (lua_isnumber(L, -1))
        {
            // Is a number or convertable to a number.
            long long val = luaL_checkint(L, -1);
            cmd_builder << val;
            ptr = (real_set.*f)(field, &val, sizeof(long long)).release();
        }
        else if (lua_isstring(L, -1) && !lua_isnumber(L, -1))
        {
            // Is a string that is not also a number.
            std::string val(lua_to_string(L, -1));
            cmd_builder << "'" << val << "'";
            ptr = (real_set.*f)(field, val.c_str(), val.size()).release();
        }
        else
        {
            // Must be a bson object.
            lj::Bson& val = Lunar<logjamd::Lua_bson>::check(L, -1)->real_node();
            if (lj::bson_type_is_quotable(val.type()))
            {
                // Deal with string bson objects.
                const std::string tmp(lj::bson_as_string(val));
                cmd_builder << "'" << tmp << "'";
                ptr = (real_set.*f)(field, tmp.c_str(), tmp.size()).release();
            }
            else if (lj::bson_type_is_nested(val.type()))
            {
                // Deal with document and array bson objects.
                char* binary = val.to_binary();
                size_t len = val.size();
                cmd_builder << "'" << lj::bson_as_string(val) << "'";
                ptr = (real_set.*f)(field, binary, len).release();
                delete[] binary;
            }
            else if (lj::Bson::k_null == val.type())
            {
                // Deal with null bson objects.
                // XXX This is all wrong code, nil should be treated
                // XXX as a difference on the current set and the set of 
                // XXX values in the index.
                char* binary = val.to_binary();
                size_t len = val.size();
                cmd_builder << "nil";
                ptr = (real_set.*f)(field, binary, len).release();
                delete[] binary;
            }
            else
            {
                // Deal with all other value types (double, int, bool, etc).
                char* binary = val.to_binary();
                size_t len = val.size();
                cmd_builder << lj::bson_as_string(val);
                ptr = (real_set.*f)(field, binary, len).release();
                delete[] binary;
            }
        }
        
        // Push the result onto the lua stack.
        logjamd::Lua_record_set* wrapper = new logjamd::Lua_record_set(ptr, cost_data);
        Lunar<logjamd::Lua_record_set>::push(L, wrapper, true);
        
        // Finish the debug info collection.
        cmd_builder << ")";
        const std::string k_command(cmd_builder.str());

        cost_data->push_child("", lj::bson_new_cost(k_command,
                                                    timer.elapsed(),
                                                    ptr->raw_size(),
                                                    ptr->size()));
    }
} // namespace

namespace logjamd
{
    const char Lua_record_set::LUNAR_CLASS_NAME[] = "Record_set";
    Lunar<Lua_record_set>::RegType Lua_record_set::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Lua_record_set, mode_and),
    LUNAR_MEMBER_METHOD(Lua_record_set, mode_or),
    LUNAR_MEMBER_METHOD(Lua_record_set, include),
    LUNAR_MEMBER_METHOD(Lua_record_set, exclude),
    LUNAR_MEMBER_METHOD(Lua_record_set, equal),
    LUNAR_MEMBER_METHOD(Lua_record_set, greater),
    LUNAR_MEMBER_METHOD(Lua_record_set, lesser),
    LUNAR_MEMBER_METHOD(Lua_record_set, contains),
    LUNAR_MEMBER_METHOD(Lua_record_set, tagged),
    LUNAR_MEMBER_METHOD(Lua_record_set, records),
    LUNAR_MEMBER_METHOD(Lua_record_set, first),
    LUNAR_MEMBER_METHOD(Lua_record_set, size),
    {0, 0, 0}
    };

    Lua_record_set::Lua_record_set(lua_State* L) : filter_(NULL), costs_(NULL)
    {
        Storage *ptr = Lunar<Storage>::check(L, -1);
        filter_ = ptr->real_storage(L).none().release();
        costs_ = new lj::Bson();
    }

    Lua_record_set::Lua_record_set(lj::Record_set* filter, lj::Bson* cost_data) : filter_(filter), costs_(cost_data)
    {
    }

    Lua_record_set::~Lua_record_set()
    {
        if (filter_)
        {
            delete filter_;
            filter_ = NULL;
        }
        
        if (costs_)
        {
            delete costs_;
            costs_ = NULL;
        }
    }

    int Lua_record_set::mode_and(lua_State* L)
    {
        real_set().set_operation(lj::Record_set::k_intersection);
        Lunar<Lua_record_set>::push(L, this, false);
        return 1;
    }

    int Lua_record_set::mode_or(lua_State* L)
    {
        real_set().set_operation(lj::Record_set::k_union);
        Lunar<Lua_record_set>::push(L, this, false);
        return 1;
    }
    
    int Lua_record_set::include(lua_State* L)
    {
        lj::Time_tracker timer;
        
        // Build the command executed.
        std::ostringstream cmd_builder;
        cmd_builder << "include(";
        
        // Include the value.
        lj::Bson* cost_data = new lj::Bson(*costs_);
        lj::Record_set* ptr = NULL;
        if (lua_isfunction(L, -1) && !lua_iscfunction(L, -1))
        {            
            // Build the command executed.
            cmd_builder << "function(b) ... end";
            
            // Get the set to filter.
            std::list<lj::Bson*> d;
            real_set().storage().all()->items(d);
            
            // Run the filter function on each item in the set.
            int function = lua_gettop(L);
            
            // Push "this" back into the stack, to prevent GC while looping.
            Lunar<Lua_record_set>::push(L, this, true);
            
            std::set<unsigned long long> keys;
            for (auto iter = d.begin();
                 d.end() != iter;
                 ++iter)
            {
                uint64_t key = lj::bson_as_uint64((*iter)->nav("__key"));
                
                // Check to see if the key is already included.
                if (real_set().is_included(key)) 
                {
                    continue;
                }
                
                // Run the function.
                lua_pushvalue(L, function);
                Lunar<Lua_bson>::push(L, new Lua_bson(*iter, true), true);
                int res = lua_pcall(L, 1, 1, 0);
                if (res == 0)
                {
                    if (lua_toboolean(L, -1))
                    {
                        keys.insert(key);
                    }
                }
                lua_pop(L, 1); // pop off the result.
            }
            lua_pop(L, 2); // pop off the function and the GC protection.
            
            // Include the key
            ptr = real_set().include_keys(keys).release();
        }
        else
        {
            int key = luaL_checkint(L, -1);
            
            cmd_builder << key;
                        
            // Include the key.
            ptr = real_set().include_key(key).release();
        }
        Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
        Lunar<Lua_record_set>::push(L, wrapper, true);
        
        // Finish the debug info collection.
        cmd_builder << ")";
        const std::string k_command(cmd_builder.str());

        cost_data->push_child("", lj::bson_new_cost(k_command,
                                                    timer.elapsed(),
                                                    ptr->raw_size(),
                                                    ptr->size()));
        
        return 1;
    }
    
    int Lua_record_set::exclude(lua_State* L)
    {
        lj::Time_tracker timer;
        timer.start();
        
        // Build the command executed.
        std::ostringstream cmd_builder;
        cmd_builder << "exclude(";
        
        // Include the value.
        lj::Bson* cost_data = new lj::Bson(*costs_);
        lj::Record_set* ptr = NULL;
        if (lua_isfunction(L, -1) && !lua_iscfunction(L, -1))
        {
            // Build the command executed.
            cmd_builder << "function(b) ... end";
            
            // Get the set to filter.
            std::list<lj::Bson*> d;
            real_set().items(d);
            
            // Run the filter function on each item in the set.
            int function = lua_gettop(L);
            
            // Push "this" back into the stack, to prevent GC while looping.
            Lunar<Lua_record_set>::push(L, this, true);
            
            std::set<unsigned long long> keys;
            for (auto iter = d.begin();
                 d.end() != iter;
                 ++iter)
            {
                uint64_t key = lj::bson_as_uint64((*iter)->nav("__key"));
                
                // Run the function.
                lua_pushvalue(L, function);
                Lunar<Lua_bson>::push(L, new Lua_bson(*iter, true), false);
                int res = lua_pcall(L, 1, 1, 0);
                if (res == 0)
                {
                    if (lua_toboolean(L, -1))
                    {
                        keys.insert(key);
                    }
                }
                else
                {
                    keys.insert(key);
                }
                lua_pop(L, 1); // pop off result.
            }
            lua_pop(L, 2); // pop off function and GC guard.
            
            // Include the key
            ptr = real_set().exclude_keys(keys).release();
        }
        else
        {
            int key = luaL_checkint(L, -1);
            
            cmd_builder << key;
            
            // Include the key.
            ptr = real_set().exclude_key(key).release();
        }
        Lua_record_set* wrapper = new Lua_record_set(ptr, cost_data);
        Lunar<Lua_record_set>::push(L, wrapper, true);
        
        // Finish the debug info collection.
        cmd_builder << ")";
        const std::string k_command(cmd_builder.str());
        cost_data->push_child("", lj::bson_new_cost(k_command,
                                                    timer.elapsed(),
                                                    ptr->raw_size(),
                                                    ptr->size()));
        
        return 1;
    }
        
    int Lua_record_set::equal(lua_State* L)
    {
        filter(L,
               real_set(),
               &lj::Record_set::equal,
               "equal",
               *costs_);
        return 1;
    }

    int Lua_record_set::greater(lua_State* L)
    {
        filter(L,
               real_set(),
               &lj::Record_set::greater,
               "greater",
               *costs_);
        return 1;
    }
    
    int Lua_record_set::lesser(lua_State* L)
    {
        filter(L,
               real_set(),
               &lj::Record_set::lesser,
               "lesser",
               *costs_);
        return 1;
    }
    
    int Lua_record_set::contains(lua_State* L)
    {
        text_filter(L,
                    real_set(),
                    &lj::Record_set::contains,
                    "contains",
                    *costs_);
        return 1;
    }

    int Lua_record_set::tagged(lua_State* L)
    {
        text_filter(L,
                    real_set(),
                    &lj::Record_set::tagged,
                    "tagged",
                    *costs_);
        return 1;
    }

    int Lua_record_set::records(lua_State* L)
    {
        lj::Time_tracker timer;

        lj::Bson* cost_data = new lj::Bson(*costs_);
        const std::string k_command("records()");

        std::list<lj::Bson *> d;
        int h = 0;
        real_set().items(d);
        lua_newtable(L);
        for (std::list<lj::Bson *>::const_iterator iter = d.begin();
             iter != d.end();
             ++iter)
        {
            Lunar<Lua_bson>::push(L, new Lua_bson(*iter, true), true);
            lua_rawseti(L, -2, ++h);
        }

        logjamd::lua::result_push(L,
                                  k_command,
                                  k_command,
                                  cost_data,
                                  NULL,
                                  timer);

        return 1;
    }

    int Lua_record_set::first(lua_State* L)
    {
        lj::Time_tracker timer;

        lj::Bson* cost_data = new lj::Bson(*costs_);
        const std::string k_command("first()");

        if (real_set().size() < 1)
        {
            lua_pushnil(L);
            return 1;
        }
        lj::Bson *d = new lj::Bson();
        real_set().first(*d);
        Lunar<Lua_bson>::push(L, new Lua_bson(d, true), true);

        logjamd::lua::result_push(L,
                                  k_command,
                                  k_command,
                                  cost_data,
                                  NULL,
                                  timer);

        return 1;
    }

    int Lua_record_set::size(lua_State* L)
    {
        lua_pushinteger(L, real_set().size());
        return 1;
    }    
};
