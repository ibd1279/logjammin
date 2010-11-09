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
#include "logjamd/lua_config.h"

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

    namespace lua
    {
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

    void logjam_lua_init(lua_State* L, lj::Bson* config) {
        // Load the Bson class into lua.
        Lunar<logjamd::Lua_bson>::Register(L);
        
        // load standard lj functions.
        lua_register(L, "send_set",
                     &send_set);
        lua_register(L, "send_item",
                     &logjamd::lua::send_item);
        lua_register(L, "print",
                     &logjamd::lua::print);

        // register the configuration api.
        logjamd::lua::register_config_api(L, config);

        // Register the object model.
        Lunar<logjamd::Lua_record_set>::Register(L);
        Lunar<logjamd::Lua_storage>::Register(L);
        
        // Build the default storage object.
        logjamd::lua::load_autoload_storage(L, config);
        
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
