#pragma once
/*!
 \file lua/Uuid.h
 \brief lua uuid header.
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

#include "lj/Uuid.h"
#include "lua/lunar.h"

namespace lua
{
    //! Lua bridge for Uuid Objects.
    /*!
     \par
     Provides a representation of Uuid objects inside Lua.
     */
    class Uuid
    {
    private:
        lj::Uuid id_;
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<Uuid>::RegType LUNAR_METHODS[];
        Uuid(lua_State* L);
        Uuid(const lj::Uuid& val);
        virtual ~Uuid();
        lj::Uuid& id();
        int __le(lua_State* L);
        int __lt(lua_State* L);
        int __eq(lua_State* L);
        int __tostring(lua_State* L);
        int __index(lua_State* L);
        int key(lua_State* L);
    }; // class Uuid
}; // namespace lua
