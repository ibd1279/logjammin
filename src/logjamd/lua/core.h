#pragma once
/*!
 \file logjamd/lua/core.h
 \brief Core logjamd server functionality.
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
#include "lj/Time_tracker.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include <string>

namespace logjamd
{
    //! Enumeration of mutable modes.
    /*!
     \par
     These mutable modes are used by the lua functions to check
     the permisibility of actions.
     */
    enum class Mutable_mode : unsigned int {
        k_config,     //!< Configration mutable mode.
        k_readonly,   //!< Read only mutable mode. No writes allowed.
        k_readwrite   //!< Read write mutable mode. All actions allowed.
    }; // enum class logjamd::lua::Mutable_mode

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
        bool test = (check_mutable_mode(config, Mutable_mode::k_config) ||
                     check_mutable_mode(config, Mutable_mode::k_readonly) ||
                     check_mutable_mode(config, Mutable_mode::k_readwrite));
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
        bool test = (check_mutable_mode(config, Mutable_mode::k_readonly) ||
                     check_mutable_mode(config, Mutable_mode::k_readwrite));
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
        bool test = check_mutable_mode(config, Mutable_mode::k_readwrite);
        if (!test)
        {
            lj::Log::notice.log("Writable test when not in a write mode for [%s].")
                    << action << lj::Log::end;
        }

        return test;
    }

    namespace lua
    {
        //! Register the configuration API into the lua state.
        /*!
         \par
         Registers the configuration api into the lua state. The APIs
         registered by this method support server and storage
         configuraton.
         \note The config pointer.
         The config pointer must be a long-lived pointer -- at least
         as long as the lua state. The reason is that the config pointer
         is used as an upvalue for all the registered functions.
         \param L The lua state to register the functions into.
         \param config Pointer to the server configuration object.
        */
        void register_config_api(lua_State* L, lj::Bson* config);

        //! Load the storage configured to be auto loaded.
        /*!
         \par Code location.
         This function exists in the lua configuration APIs because
         it depends on the functionality to store and load storage
         events.
         \param L the lua state.
         \param config Pointer to the server configuration object.
         */
        void load_autoload_storage(lua_State* L, const lj::Bson* config);

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

        //! Create a command name from a cost structure.
        /*!
         \par
         Take the given lj::Bson document of costs and create a
         simple combine command.
         \param prefix String to prepend before the command.
         \param suffix String to append after the command.
         \param costs The lj::Bson document of costs.
         \return The final command.
         */
        std::string command_from_costs(const std::string& prefix,
                                       const std::string& suffix,
                                       const lj::Bson& costs);

        //! Push cost data into the response object.
        /*!
         Gets the response object from the current sandbox, appends
         the new result.
         \note The memory associated with \c cost_data
         and \c items becomes owned by the result.
         \param L The lua closure associated with the sandbox.
         \param full_cmd The command that generated this result.
         \param current_cmd The command that caused result_push to be called.
         \param cost_data The cost of generating this result. Cost data
         will be allocated if \c cost_data is \c NULL.
         \param items The items of the result. Results will be ommited
         if \c items is an empty lj::Bson or \c NULL.
         \param timer The timer to use for tracking the cost of "pushing
         the result".
         \return number of items added to the top of the stack -- always 0.
         */
        int result_push(lua_State* L,
                        const std::string& full_cmd,
                        const std::string& current_cmd,
                        lj::Bson* cost_data,
                        lj::Bson* items,
                        lj::Time_tracker& timer);

        //! Get the server configuration object from the lua state.
        /*!
         Get the current server configuration from the sandbox. This
         does not change the stack.
         \param L The lua state.
         \return A reference to the server configuration.
         */
        const lj::Bson& get_configuration(lua_State* L);

        //! Fail out of the current lua command.
        /*!
         Records the time spent executing the current command, then
         it throws a lua error to end execution of the current command.
         \param L The lua state.
         \param command The command being executed.
         \param msg The error message.
         \param timer The timer to use for tracking the cost of the failed
         action.
         */
        int fail(lua_State* L,
                 const std::string& command,
                 const std::string& msg,
                 lj::Time_tracker& timer);
    }; // namespace logjamd::lua
}; // namespace logjamd
