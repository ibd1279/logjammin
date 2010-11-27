#pragma once
/*!
 \file lua_shared.h
 \brief Logjam server shared lua functions header.
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

#include <string>

namespace logjamd
{
    namespace lua
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
    }; // namespace logjamd::lua
}; // namespace logjamd
