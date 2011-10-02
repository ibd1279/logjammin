/*!
 \file lua/Uuid.cpp
 \brief lua uuid implementation.
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

#include "lua/Uuid.h"

namespace lua
{
    const char Uuid::LUNAR_CLASS_NAME[] = "Uuid";
    Lunar<Uuid>::RegType Uuid::LUNAR_METHODS[] = {
        LUNAR_METHOD(Uuid, __tostring)
        ,LUNAR_METHOD(Uuid, __le)
        ,LUNAR_METHOD(Uuid, __lt)
        ,LUNAR_METHOD(Uuid, __eq)
        ,LUNAR_METHOD(Uuid, key)
        ,{0, 0}
    };

    Uuid::Uuid(lua_State* L) : id_()
    {
        int top = lua_gettop(L);
        if (1 == top)
        {
            switch (lua_type(L, -1))
            {
                case LUA_TNIL:
                    id_ = lj::Uuid::k_nil;
                    break;
                case LUA_TNUMBER:
                    id_ = lj::Uuid(lua_tointeger(L, -1));
                    break;
                case LUA_TSTRING:
                    id_ = lj::Uuid(as_string(L, -1));
                    break;
                case LUA_TLIGHTUSERDATA:
                case LUA_TUSERDATA:
                    id_ = Lunar<Uuid>::check(L, -1)->id();
                    break;
                default:
                    lua_pushstring(L, "Uuid expected nil, number, string, or a Uuid for the first argument.");
                    lua_error(L);
            }
        }
        else if(1 < top)
        {
            lua_pushstring(L, "Uuid expected a single argument.");
            lua_error(L);
        }
        // 0 args uses the default constructor above.
    }

    Uuid::Uuid(const lj::Uuid& val) : id_(val)
    {
    }

    Uuid::~Uuid()
    {
    }

    lj::Uuid& Uuid::id()
    {
        return id_;
    }

    int Uuid::__le(lua_State* L)
    {
        Uuid* right = Lunar<Uuid>::check(L, -1);
        lua_pushboolean(L, id() <= right->id());
        return 1;
    }

    int Uuid::__lt(lua_State* L)
    {
        Uuid* right = Lunar<Uuid>::check(L, -1);
        lua_pushboolean(L, id() < right->id());
        return 1;
    }

    int Uuid::__eq(lua_State* L)
    {
        Uuid* right = Lunar<Uuid>::check(L, -1);
        lua_pushboolean(L, id_ == right->id());
        return 1;
    }

    int Uuid::__tostring(lua_State* L)
    {
        lua_pushstring(L, static_cast<std::string>(id_).c_str());
        return 1;
    }

    int Uuid::key(lua_State* L)
    {
        lua_pushinteger(L, static_cast<uint64_t>(id_));
        return 1;
    }
}; // namespace lua
