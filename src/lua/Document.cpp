/*!
 \file lua/Document.cpp
 \brief lua Document implementation
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

#include "lua/Bson.h"
#include "lua/Document.h"
#include "lua/Uuid.h"

#include "cryptopp/secblock.h"

#include <iostream>

namespace lua
{
    const char Document::LUNAR_CLASS_NAME[] = "Document";
    Lunar<Document>::RegType Document::LUNAR_METHODS[] = {
        LUNAR_METHOD(Document, parent)
        ,LUNAR_METHOD(Document, vclock)
        ,LUNAR_METHOD(Document, version)
        ,LUNAR_METHOD(Document, key)
        ,LUNAR_METHOD(Document, id)
        ,LUNAR_METHOD(Document, suppress)
        ,LUNAR_METHOD(Document, dirty)
        ,LUNAR_METHOD(Document, get)
        ,LUNAR_METHOD(Document, exists)
        ,LUNAR_METHOD(Document, wash)
        ,LUNAR_METHOD(Document, rekey)
        ,LUNAR_METHOD(Document, branch)
        ,LUNAR_METHOD(Document, set)
        ,LUNAR_METHOD(Document, push)
        ,LUNAR_METHOD(Document, increment)
        ,LUNAR_METHOD(Document, encrypt)
        ,LUNAR_METHOD(Document, decrypt)
        ,LUNAR_METHOD(Document, __tostring)
        ,LUNAR_METHOD(Document, __index)
        ,{0, 0}
    };

    Document::Document(lua_State* L) :
            doc_(new lj::Document()),
            gc_(true)
    {
    }

    Document::Document(lj::Document* val, bool gc) :
            doc_(val),
            gc_(gc)
    {
    }

    Document::~Document()
    {
        if (doc_ && gc_)
        {
            delete doc_;
        }
    }

    int Document::parent(lua_State* L)
    {
        Lunar<Uuid>::push(L, new Uuid(doc_->parent()), true);
        return 1;
    }

    int Document::vclock(lua_State* L)
    {
        Lunar<Bson_ro>::push(L, new Bson_ro(doc_->vclock()), true);
        return 1;
    }

    int Document::version(lua_State* L)
    {
        int32_t ver = doc_->version();
        lua_pushinteger(L, ver);
        return 1;
    }

    int Document::key(lua_State* L)
    {
        uint64_t key = doc_->key();
        lua_pushnumber(L, static_cast<double>(key));
        return 1;
    }

    int Document::id(lua_State* L)
    {
        Lunar<Uuid>::push(L, new Uuid(doc_->id()), true);
        return 1;
    }

    int Document::suppress(lua_State* L)
    {
        int top = lua_gettop(L);
        if (0 == top)
        {
            lua_pushboolean(L, doc_->suppress());
            return 1;
        }
        else if (1 == top)
        {
            // XXX Change this to get the server from somewhere.
            doc_->suppress(lj::Uuid::k_nil, lua_toboolean(L, -1));
            return 0;
        }
        else
        {
            lua_pushstring(L, "Expected 0 or 1 argument.");
            lua_error(L);
        }
        return 0;
    }

    int Document::dirty(lua_State* L)
    {
        lua_pushboolean(L, doc_->dirty());
        return 1;
    }

    int Document::get(lua_State* L)
    {
        int top = lua_gettop(L);

        std::string tmp(as_string(L, -1));
        try
        {
            if (top == 1)
            {
                const lj::bson::Node& n(doc_->get(tmp));
                Lunar<Bson_ro>::push(L, new Bson_ro(n), true);
            }
            else
            {
                const lj::bson::Node& n(doc_->get());
                Lunar<Bson_ro>::push(L, new Bson_ro(n), true);
            }
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 1;
    }

    int Document::exists(lua_State* L)
    {
        std::string tmp(as_string(L, -1));
        try
        {
            const lj::bson::Node& n(doc_->get());
            lua_pushboolean(L, n.exists(tmp));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 1;
    }

    int Document::wash(lua_State* L)
    {
        doc_->wash();
        return 0;
    }

    int Document::rekey(lua_State* L)
    {
        uint64_t key = lua_tointeger(L, -1);
        // XXX Change this to get the server from somewhere.
        doc_->rekey(lj::Uuid::k_nil, key);
        return 0;
    }

    int Document::branch(lua_State* L)
    {
        int top = lua_gettop(L);

        lj::Document* dup;
        if (top == 1)
        {
            uint64_t key = lua_tointeger(L, -1);
            // XXX Change this to get the server from somewhere.
            dup = doc_->branch(lj::Uuid::k_nil, key);
        }
        else
        {
            // XXX Change this to get the server from somewhere.
            dup = doc_->branch(lj::Uuid::k_nil, doc_->key());
        }
        Lunar<lua::Document>::push(L,
                new lua::Document(dup, true),
                true);
        return 1;
    }

    int Document::set(lua_State* L)
    {
        std::string tmp(as_string(L, -2));
        Bson* val = Lunar<Bson>::check(L, -1);
        try
        {
            // XXX Change this to get the server from somewhere.
            doc_->set(lj::Uuid::k_nil, tmp, new lj::bson::Node(val->node()));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Document::push(lua_State* L)
    {
        std::string tmp(as_string(L, -2));
        Bson* val = Lunar<Bson>::check(L, -1);
        try
        {
            // XXX Change this to get the server from somewhere.
            doc_->push(lj::Uuid::k_nil, tmp, new lj::bson::Node(val->node()));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Document::increment(lua_State* L)
    {
        std::string tmp(as_string(L, -2));
        int amt = lua_tointeger(L, -1);
        try
        {
            // XXX Change this to get the server from somewhere.
            doc_->increment(lj::Uuid::k_nil, tmp, amt);
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Document::encrypt(lua_State* L)
    {
        int top = lua_gettop(L);
        
        // Every arg except the first is a path to encrypt
        std::vector<std::string> paths;
        if (top > 1)
        {
            paths.resize(top - 1);
            while (top > 1)
            {
                // This code moves the item at lua index 2 and puts it
                // in C index 0.
                paths[top - 2] = as_string(L, -1);
                lua_pop(L, 1);
                top = lua_gettop(L);
            }
        }

        // use the provided arg to look up the crypto keys.
        lua_getglobal(L, "get_crypto_key");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);

        // Get the returned value. nil will cause an error bumping us out.
        Bson_ro* val = Lunar<Bson_ro>::check(L, -1);
        std::string key_name(as_string(L, -2));

        // Create a copy of the data. It is possible that the pop below
        // will cause the returned value to GC.
        lj::bson::Binary_type bt;
        uint32_t sz;
        const uint8_t* key_data = lj::bson::as_binary(val->node(), &bt, &sz);
        CryptoPP::SecByteBlock key(key_data, sz);
        lua_pop(L, 2);

        try
        {
            // XXX Change this to get the server from somewhere.
            doc_->encrypt(lj::Uuid::k_nil,
                    key.data(),
                    key.size(),
                    key_name,
                    paths);
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Document::decrypt(lua_State* L)
    {
        // use the provided arg to look up the crypto keys.
        lua_getglobal(L, "get_crypto_key");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 1);

        // Get the returned value. nil will cause an error bumping us out.
        Bson_ro* val = Lunar<Bson_ro>::check(L, -1);
        std::string key_name(as_string(L, -2));

        // Create a copy of the data. It is possible that the pop below
        // will cause the returned value to GC.
        lj::bson::Binary_type bt;
        uint32_t sz;
        const uint8_t* key_data = lj::bson::as_binary(val->node(), &bt, &sz);
        CryptoPP::SecByteBlock key(key_data, sz);
        lua_pop(L, 2);

        try
        {
            doc_->decrypt(key.data(),
                    key.size(),
                    key_name);
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Document::__tostring(lua_State* L)
    {
        lua_pushstring(L, static_cast<std::string>(*doc_).c_str());
        return 1;
    }

    int Document::__index(lua_State* L)
    {
        return get(L);
    }
}; // namespace lua
