/*!
 \file lua_shared.cpp
 \brief Logjam server shared lua components.
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

#include "logjamd/lua_shared.h"

#include "logjamd/Lua_bson.h"
#include "logjamd/Lua_record_set.h"
#include "logjamd/Lua_storage.h"
#include "logjamd/lua_config.h"
#include "lj/Base64.h"
#include "lj/Logger.h"
#include "lj/Storage.h"
#include "lj/Storage_factory.h"
#include "lj/Time_tracker.h"
#include "build/default/config.h"

#include <string>
#include <sstream>

namespace logjamd
{
    namespace lua
    {
        bool check_mutable_mode(const lj::Bson& config, const Mutable_mode mode)
        {
            const lj::Bson& tmp = config.nav("server/mode");
            return (mode == static_cast<Mutable_mode>(lj::bson_as_int64(tmp)));
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

        int result_push(lua_State* L,
                        const std::string& full_cmd,
                        const std::string& current_cmd,
                        lj::Bson* cost_data,
                        lj::Bson* items,
                        lj::Time_tracker& timer)
        {
            // {}
            logjamd::lua::sandbox_get(L, "lj__response"); // {response}
            lj::Bson& response = Lunar<Lua_bson>::check(L, -1)->real_node();
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
            const lj::Bson& config = Lunar<Lua_bson>::check(L, -1)->real_node();
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
