#pragma once
/*!
 \file lua/Bson.h
 \brief lua bson header.
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
#include "lua/lunar.h"
#include "lua/Uuid.h"

namespace lua
{
    class Bson
    {
    private:
        std::shared_ptr<lj::bson::Node> node_;
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<Bson>::RegType LUNAR_METHODS[];
        Bson(lua_State* L);
        Bson(const lj::bson::Node& val);
        Bson(std::shared_ptr<lj::bson::Node>& root,
                const std::string& path);
        virtual ~Bson();
        lj::bson::Node& node();
        int type(lua_State* L);
        int nullify(lua_State* L);
        virtual int path(lua_State* L);
        int set_null(lua_State* L);
        int set_document(lua_State* L);
        int set_array(lua_State* L);
        int set_boolean(lua_State* L);
        int set_string(lua_State* L);
        int set_int32(lua_State* L);
        int set_int64(lua_State* L);
        int set_uuid(lua_State* L);
        int as_string(lua_State* L);
        int as_nil(lua_State* L);
        int as_table(lua_State* L);
        int as_number(lua_State* L);
        int as_boolean(lua_State* L);
        int as_uuid(lua_State* L);
        int __tostring(lua_State* L);
        int __index(lua_State* L);
    }; // class Bson

    class Bson_ro : public Bson
    {
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<Bson_ro>::RegType LUNAR_METHODS[];
        Bson_ro(lua_State* L);
        Bson_ro(const lj::bson::Node& val);
        virtual ~Bson_ro();
        virtual int path(lua_State* L);
    }; // class Bson_ro

}; // namespace lua
