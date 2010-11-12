#pragma once
/*!
 \file logjamd_lua.h
 \brief Logjam server lua functions header.
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

#include "lj/Bson.h"
#include "lj/Logger.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include <cstring>
#include <string>

namespace logjamd
{
    void set_logging_levels(const lj::Bson& config);

    namespace lua
    {
        //! Enumeration of mutable modes.
        /*!
         \par
         These mutable modes are used by the lua functions to check
         the permisibility of actions.
         */
        enum Mutable_mode {
            k_config,     //!< Configration mutable mode.
            k_readonly,   //!< Read only mutable mode. No writes allowed.
            k_readwrite   //!< Read write mutable mode. All actions allowed.
        };

        //! Check to see if the configuration is in a given mutable mode.
        /*!
         \param config The config file to test.
         \param mode The mode to test.
         \return true if the mode matches, false otherwise.
         */
        bool check_mutable_mode(const lj::Bson& config, const Mutable_mode mode);

        //! Shortcut to checking for the configurable mode.
        inline bool is_mutable_config(const lj::Bson& config, const std::string& action)
        {
            bool test = (check_mutable_mode(config, k_config) ||
                         check_mutable_mode(config, k_readonly) ||
                         check_mutable_mode(config, k_readwrite));
            if (!test)
            {
                lj::Log::notice.log("Configurable test when not in a config mode for [%s].")
                        << action << lj::Log::end;
            }

            return test;
        }

        //! Shortcut to checking the readable mode.
        inline bool is_mutable_read(const lj::Bson& config, const std::string& action)
        {
            bool test = (check_mutable_mode(config, k_readonly) ||
                         check_mutable_mode(config, k_readwrite));
            if (!test)
            {
                lj::Log::notice.log("Readable test when not in a read mode for [%s].")
                        << action << lj::Log::end;
            }

            return test;
        }

        //! Shortcut to checking the writable mode.
        inline bool is_mutable_write(const lj::Bson& config, const std::string& action)
        {
            bool test = (check_mutable_mode(config, k_readonly) ||
                         check_mutable_mode(config, k_readwrite));
            if (!test)
            {
                lj::Log::notice.log("Writable test when not in a write mode for [%s].")
                        << action << lj::Log::end;
            }

            return test;
        }
    };
    //! Put the environment table for this identifier ontop of the stack.
    /*!
     Creates the environment if it doesn't already exist.
     \param L The lua closure associated with the sandbox.
     \return number of items added to the top of the stack -- always 1.
     */
    int sandbox_push(lua_State* L);

    //! Get a value from the sandbox environment.
    /*!
     returns nil if the value does not exist.
     \param L The lua closure associated with the sandbox.
     \param key The key of the value to load.
     \return number of items added to the top of the stack -- always 1.
     */
    int sandbox_get(lua_State* L, const std::string& key);
    

    //! Initialize the lua state for the server process.
    /*!
     \par
     This is primarily responsible for getting the cached environment for
     a connection, and leaving it ontop of the stack.
     \param L The lua state.
     \param config The data directory.
     */
    void logjam_lua_init(lua_State* L, lj::Bson* config);
    
    //! Execute an event.
    void get_event(lua_State* L, const std::string& db_name, const std::string& event);
    
    //! Push a bson object onto the replication queue and return the name.
    const std::string push_replication_record(lua_State* L, const lj::Bson& b);
    
    void push_replication_command(lua_State* L,
                                  const std::string& action,
                                  const std::string& dbname,
                                  const std::string& obj);    
    
    //! Put a result set on the response.
    /*!
     \par
     Populates the item field on the response.
     \par
     Expects a Lua_record_set object on top of the stack.
     \param L The lua state.
     \return 0
     */
    int send_set(lua_State* L);
};
