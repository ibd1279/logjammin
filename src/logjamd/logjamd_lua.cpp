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

#include "logjamd/lua/core.h"
#include "logjamd/lua/Bson.h"
#include "logjamd/lua/Storage.h"
#include "logjamd/lua/Record_set.h"
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
    void set_loglevel(lj::Log& log, const bool enable)
    {
        if (enable)
        {
            log.enable();
        }
        else
        {
            log.disable();
        }
    }
}; // namespace (anonymous)

namespace logjamd
{
    void set_logging_levels(const lj::Bson& config)
    {
        set_loglevel(lj::Log::debug,
                     lj::bson_as_boolean(config.nav("logging/debug")));
        set_loglevel(lj::Log::info,
                     lj::bson_as_boolean(config.nav("logging/info")));
        set_loglevel(lj::Log::notice,
                     lj::bson_as_boolean(config.nav("logging/notice")));
        set_loglevel(lj::Log::warning,
                     lj::bson_as_boolean(config.nav("logging/warning")));
        set_loglevel(lj::Log::error,
                     lj::bson_as_boolean(config.nav("logging/error")));
        set_loglevel(lj::Log::critical,
                     lj::bson_as_boolean(config.nav("logging/critical")));
        set_loglevel(lj::Log::alert,
                     lj::bson_as_boolean(config.nav("logging/alert")));
        set_loglevel(lj::Log::emergency,
                     lj::bson_as_boolean(config.nav("logging/emergency")));
    }

    void logjam_lua_init(lua_State* L, lj::Bson* config) {
        // Load the Bson class into lua.
        Lunar<logjamd::lua::Bson>::Register(L);

        // register the configuration api.
        logjamd::lua::register_config_api(L, config);

        // Register the object model.
        Lunar<logjamd::lua::Record_set>::Register(L);
        Lunar<logjamd::lua::Storage>::Register(L);
        
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
};
