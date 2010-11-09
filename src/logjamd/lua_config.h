#pragma once
/*!
 \file lua_config.h
 \brief Logjam server lua functions for configuration definition.
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

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

namespace logjamd
{
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
    }; // namespace logjamd::lua
}; // namespace logjamd
