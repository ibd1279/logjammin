/*!
 \file Client_command_processor.cpp
 \brief Logjam server client command implementation.
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

#include "logjamd/Client_command_processor.h"

#include "logjamd/logjamd_lua.h"
#include "logjamd/Lua_bson.h"
#include "lj/Bson.h"
#include "lj/Logger.h"
#include "lj/Time_tracker.h"

namespace logjamd
{
    Client_command_processor::Client_command_processor() : Client_processor::Client_processor()
    {
    }

    Client_command_processor::~Client_command_processor()
    {
    }

    Client_processor* Client_command_processor::logic(lj::Bson& request, Connection& connection)
    {
        lj::Time_tracker timer;
        timer.start();
        
        std::string command(lj::bson_as_string(request.nav("lj__command")));

        // Create the client's Lua thread.
        lua_State *L = lua_newthread(connection.lua());

        // Copy the global server configuration to prevent modification.
        lj::Bson server_config(connection.server_config());
        Lua_bson wrapper_config(&server_config, false);
        Lunar<Lua_bson>::push(L, &wrapper_config, false);
        lua_setglobal(L, "lj__config");
        
        // Load the request object.
        Lua_bson wrapped_request(&request, false);
        Lunar<Lua_bson>::push(L, &wrapped_request, false);
        lua_setglobal(L, "lj__request");

        // Load the response object.
        lj::Bson response;
        Lua_bson wrapped_response(&response, false);
        Lunar<Lua_bson>::push(L, &wrapped_response, false);
        lua_setglobal(L, "lj__response");

        // Load the replication log.
        lj::Bson replication;
        Lua_bson wrapped_replication(&replication, false);
        Lunar<Lua_bson>::push(L, &wrapped_replication, false);
        lua_setglobal(L, "lj__replication");
        replication.set_child("lj__command", lj::bson_new_string(""));
        replication.set_child("lj__dirty", lj::bson_new_boolean(false));

        // Load some connection specific variables.
        lua_pushstring(L, connection.ip().c_str());
        lua_setglobal(L, "connection_id");

        // Load the closure.
        luaL_loadbuffer(L,
                        command.c_str(),
                        command.size(),
                        connection.ip().c_str());
        
        // Hide the global environment.
        logjam_lua_init_connection(L, connection.ip());
        lua_setfenv(L, -2);
        
        // Execute the commands received.
        int error;
        while (true)
        {
            error = lua_resume(L, 0);
            if (LUA_YIELD != error)
            {
                // Yields loop, all other cases break.
                // this is incase I decide to do something more co-operative later.
                break;
            }
        }
        
        // Process the response/deal with errors.
        if (error)
        {
            const char* error_string = lua_tostring(L, -1);
            lj::Log::info.log("Lua error: %s") << error_string << lj::Log::end;
            response.set_child("error", lj::bson_new_string(error_string));
            response.set_child("is_ok", lj::bson_new_boolean(false));
        }
        else
        {
            response.set_child("is_ok", lj::bson_new_boolean(true));
        }
        // Clear off the stack and stop time tracking.
        lua_pop(L, 1);
        timer.stop();
        
        // Record server performance metrics.
        response.set_child("time/elapsed_usecs", lj::bson_new_uint64(timer.elapsed()));

        lj::Log::info.log("Replication Log for %s: %s")
                << lj::bson_as_pretty_string(server_config.nav("replication/peers"))
                << lj::bson_as_pretty_string(replication) << lj::Log::end;
        
        char* buffer = response.to_binary();
        connection.add_bytes(buffer, response.size());
        delete[] buffer;
        connection.set_writing(true);

        // return this for the next command.
        return this;
    }
};

