/*!
 \file Lua_storage.cpp
 \brief Logjamd lj::Storage wrapper for lua implementation.
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

#include "logjamd/Lua_storage.h"

#include "logjamd/Lua_bson.h"
#include "logjamd/Lua_record_set.h"
#include "build/default/config.h"
#include "logjamd/logjamd_lua.h"
#include "lj/Storage_factory.h"

#include <string>
#include <sstream>

namespace logjamd
{
    const char Lua_storage::LUNAR_CLASS_NAME[] = "Storage";
    Lunar<Lua_storage>::RegType Lua_storage::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Lua_storage, all),
    LUNAR_MEMBER_METHOD(Lua_storage, none),
    LUNAR_MEMBER_METHOD(Lua_storage, at),
    LUNAR_MEMBER_METHOD(Lua_storage, place),
    LUNAR_MEMBER_METHOD(Lua_storage, remove),
    {0, 0, 0}
    };
    
    Lua_storage::Lua_storage(const std::string& dbname) : storage_(NULL)
    {
        storage_ = lj::Storage_factory::produce(dbname);
    }
    
    Lua_storage::Lua_storage(lua_State* L) : storage_(NULL)
    {
        std::string dbname(lua_to_string(L, -1));
        storage_ = lj::Storage_factory::produce(dbname);
    }
    
    Lua_storage::~Lua_storage()
    {
    }
    
    int Lua_storage::all(lua_State* L)
    {
        lj::Record_set* ptr = real_storage().all().release();
        Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        return 1;
    }
    
    int Lua_storage::none(lua_State* L)
    {
        lj::Record_set* ptr = real_storage().none().release();
        Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        return 1;
    }
    
    int Lua_storage::at(lua_State* L)
    {
        lj::Record_set* ptr = real_storage().at(luaL_checkint(L, -1)).release();
        Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        return 1;
    }
    
    int Lua_storage::place(lua_State* L)
    {
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        try
        {
            real_storage().place(ptr->real_node());
        }
        catch(lj::Exception* ex)
        {
            luaL_error(L, "Unable to place content. %s", ex->to_string().c_str());
        }
        return 0;
    }
    
    int Lua_storage::remove(lua_State* L)
    {
        Lua_bson* ptr = Lunar<Lua_bson>::check(L, -1);
        real_storage().remove(ptr->real_node());
        return 0;
    }
}; // namespace logjamd
