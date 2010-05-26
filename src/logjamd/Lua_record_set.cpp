/*!
 \file Lua_record_set.cpp
 \brief Logjamd lj::Record_set wrapper implement.
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

#include "Lua_record_set.h"

#include "logjamd/Lua_storage.h"
#include "logjamd/Lua_bson.h"
#include "lj/Bson.h"
#include "lj/Storage.h"
#include "lj/Time_tracker.h"

#include <string>
#include <memory>

namespace logjamd
{
    const char Lua_record_set::LUNAR_CLASS_NAME[] = "Record_set";
    Lunar<Lua_record_set>::RegType Lua_record_set::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Lua_record_set, mode_and),
    LUNAR_MEMBER_METHOD(Lua_record_set, mode_or),
    LUNAR_MEMBER_METHOD(Lua_record_set, include),
    LUNAR_MEMBER_METHOD(Lua_record_set, exclude),
    LUNAR_MEMBER_METHOD(Lua_record_set, equal),
    LUNAR_MEMBER_METHOD(Lua_record_set, greater),
    LUNAR_MEMBER_METHOD(Lua_record_set, lesser),
    LUNAR_MEMBER_METHOD(Lua_record_set, search),
    LUNAR_MEMBER_METHOD(Lua_record_set, tagged),
    LUNAR_MEMBER_METHOD(Lua_record_set, records),
    LUNAR_MEMBER_METHOD(Lua_record_set, first),
    LUNAR_MEMBER_METHOD(Lua_record_set, size),
    {0, 0, 0}
    };

    Lua_record_set::Lua_record_set(lua_State* L) : filter_(NULL)
    {
        Lua_storage *ptr = Lunar<Lua_storage>::check(L, -1);
        filter_ = ptr->real_storage().none().release();
    }

    Lua_record_set::Lua_record_set(lj::Record_set *filter) : filter_(filter)
    {
    }

    Lua_record_set::~Lua_record_set()
    {
        if (filter_)
        {
            delete filter_;
        }
    }

    int Lua_record_set::mode_and(lua_State* L)
    {
        real_set().set_operation(lj::Record_set::k_intersection);
        Lunar<Lua_record_set>::push(L, this, false);
        return 1;
    }

    int Lua_record_set::mode_or(lua_State* L)
    {
        real_set().set_operation(lj::Record_set::k_union);
        Lunar<Lua_record_set>::push(L, this, false);
        return 1;
    }
    
    int Lua_record_set::include(lua_State* L)
    {
        int key = luaL_checkint(L, -1);
        lj::Record_set* ptr = real_set().include_key(key).release();
        Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        return 1;
    }
    
    int Lua_record_set::exclude(lua_State* L)
    {
        int key = luaL_checkint(L, -1);
        lj::Record_set* ptr = real_set().exclude_key(key).release();
        Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        return 1;
    }
    
    namespace
    {
        void filter(lua_State* L, 
                    lj::Record_set& real_set,
                    std::auto_ptr<lj::Record_set> (lj::Record_set::*f)(const std::string&, const void* const, const size_t) const)
        {
            std::string field(lua_to_string(L, -2));
            lj::Record_set* ptr;
            if (lua_isstring(L, -1))
            {
                std::string val(lua_to_string(L, -1));
                ptr = (real_set.*f)(field, val.c_str(), val.size()).release();
            }
            else if (lua_isnumber(L, -1))
            {
                long long val = luaL_checkint(L, -1);
                ptr = (real_set.*f)(field, &val, sizeof(long long)).release();
            }
            else
            {
                Lua_bson* val = Lunar<Lua_bson>::check(L, -1);
                char* binary = val->real_node().to_binary();
                size_t len = val->real_node().size();
                if (lj::bson_type_is_quotable(val->real_node().type()))
                {
                    ptr = (real_set.*f)(field, binary + 4, len - 5).release();
                }
                else
                {
                    ptr = (real_set.*f)(field, binary, len).release();
                }
                delete[] binary;
            }
            Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        }
    } // namespace
    
    int Lua_record_set::equal(lua_State* L)
    {
        lj::Time_tracker timer;
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        timer.start();
        
        filter(L, real_set(), &lj::Record_set::equal);
        
        timer.stop();
        node->real_node().push_child("time/query_usecs", 
                                     lj::bson_new_uint64(timer.elapsed()));
        return 1;
    }

    int Lua_record_set::greater(lua_State* L)
    {
        lj::Time_tracker timer;
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        timer.start();
        
        filter(L, real_set(), &lj::Record_set::greater);
        
        timer.stop();
        node->real_node().push_child("time/query_usecs", 
                                     lj::bson_new_uint64(timer.elapsed()));
        return 1;
    }
    
    int Lua_record_set::lesser(lua_State* L)
    {
        lj::Time_tracker timer;
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        timer.start();
        
        filter(L, real_set(), &lj::Record_set::lesser);
        
        timer.stop();
        node->real_node().push_child("time/query_usecs", 
                                     lj::bson_new_uint64(timer.elapsed()));
        return 1;
    }
    
    int Lua_record_set::search(lua_State* L)
    {
        lj::Time_tracker timer;
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        timer.start();
        
        std::string field(lua_to_string(L, -2));
        std::string val(lua_to_string(L, -1));
        lj::Record_set* ptr = real_set().contains(field, val).release();
        Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        
        timer.stop();
        node->real_node().push_child("time/query_usecs", 
                                     lj::bson_new_uint64(timer.elapsed()));
        return 1;
    }

    int Lua_record_set::tagged(lua_State* L)
    {
        lj::Time_tracker timer;
        lua_getglobal(L, "response");
        Lua_bson* node = Lunar<Lua_bson>::check(L, -1);
        lua_pop(L, 1);
        timer.start();
        
        std::string field(lua_to_string(L, -2));
        std::string val(lua_to_string(L, -1));
        lj::Record_set* ptr = real_set().tagged(field, val).release();
        Lunar<Lua_record_set>::push(L, new Lua_record_set(ptr), true);
        
        timer.stop();
        node->real_node().push_child("time/query_usecs", 
                                     lj::bson_new_uint64(timer.elapsed()));
        return 1;
    }

    int Lua_record_set::records(lua_State* L)
    {
        std::list<lj::Bson *> d;
        int h = 0;
        real_set().items(d);
        lua_newtable(L);
        for (std::list<lj::Bson *>::const_iterator iter = d.begin();
             iter != d.end();
             ++iter)
        {
            Lunar<Lua_bson>::push(L, new Lua_bson(*iter, true), true);
            lua_rawseti(L, -2, ++h);
        }
        return 1;
    }

    int Lua_record_set::first(lua_State* L)
    {
        if (real_set().size() < 1)
        {
            lua_pushnil(L);
            return 1;
        }
        lj::Bson *d = new lj::Bson();
        real_set().first(*d);
        Lunar<Lua_bson>::push(L, new Lua_bson(d, true), true);
        return 1;
    }

    int Lua_record_set::size(lua_State* L)
    {
        lua_pushinteger(L, real_set().size());
        return 1;
    }
};