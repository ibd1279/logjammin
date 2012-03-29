/*!
 \file lua/Bson.cpp
 \brief lua bson integration.
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
#include "lua/Bson.h"
#include "lua/Uuid.h"

namespace lua
{
    const char Bson::LUNAR_CLASS_NAME[] = "Bson";
    Lunar<Bson>::RegType Bson::LUNAR_METHODS[] =
    {
        LUNAR_METHOD(Bson, type)
        ,LUNAR_METHOD(Bson, nullify)
        ,LUNAR_METHOD(Bson, path)
        ,LUNAR_METHOD(Bson, clone)
        ,LUNAR_METHOD(Bson, clone_immutable)
        ,LUNAR_METHOD(Bson, set_null)
        ,LUNAR_METHOD(Bson, set_document)
        ,LUNAR_METHOD(Bson, set_array)
        ,LUNAR_METHOD(Bson, set_boolean)
        ,LUNAR_METHOD(Bson, set_string)
        ,LUNAR_METHOD(Bson, set_int32)
        ,LUNAR_METHOD(Bson, set_int64)
        ,LUNAR_METHOD(Bson, set_uuid)
        ,LUNAR_METHOD(Bson, as_string)
        ,LUNAR_METHOD(Bson, as_nil)
        ,LUNAR_METHOD(Bson, as_table)
        ,LUNAR_METHOD(Bson, as_number)
        ,LUNAR_METHOD(Bson, as_boolean)
        ,LUNAR_METHOD(Bson, as_uuid)
        ,LUNAR_METHOD(Bson, __tostring)
        ,LUNAR_METHOD(Bson, __index)
        ,{0, 0}
    };

    Bson::Bson(lua_State* L) :
            node_(new lj::bson::Node())
    {
        int top = lua_gettop(L);
        if (top == 1)
        {
            try
            {
                if (lua_isuserdata(L, 1))
                {
                    // Try to read an existing bson object from the user
                    // data.
                    Bson* orig = Lunar<lua::Bson>::check(L, 1);
                    node_->copy_from(orig->node());
                }
                else
                {
                    // perform json parsing on the input string.
                    std::string tmp(lua::as_string(L, -1));
                    lua_pop(L, 1);
                    std::unique_ptr<lj::bson::Node> n(
                            lj::bson::parse_string(tmp));
                    node_->copy_from(*n);
                }
            }
            catch (lj::Exception& ex)
            {
                lua_pushstring(L, ex.str().c_str());
                lua_error(L);
            }
        }
    }

    Bson::Bson(const lj::bson::Node& val) :
            node_(new lj::bson::Node(val))
    {
    }

    Bson::Bson(std::shared_ptr<lj::bson::Node>& root,
            const std::string& path) : 
            node_(root, root->path(path))
    {
    }

    Bson::~Bson()
    {
    }

    lj::bson::Node& Bson::node()
    {
        return *node_;
    }

    int Bson::type(lua_State* L)
    {
        std::string tmp(lj::bson::type_string(node_->type()));
        lua_pushstring(L, tmp.c_str());
        return 1;
    }

    int Bson::nullify(lua_State* L)
    {
        node_->nullify();
        return 0;
    }

    int Bson::path(lua_State* L)
    {
        std::string tmp(lua::as_string(L, -1));
        lua_pop(L, 1);
        try
        {
            Lunar<lua::Bson>::push(L, new Bson(node_, tmp), true);
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 1;
    }

    int Bson::clone(lua_State* L)
    {
        int top = lua_gettop(L);
        try
        {
            if (top == 1)
            {
                std::string tmp(lua::as_string(L, -1));
                lua_pop(L, 1);
                Lunar<lua::Bson>::push(L, new Bson(node_->nav(tmp)), true);
            }
            else
            {
                Lunar<lua::Bson>::push(L, new Bson(*node_), true);
            }
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 1;
    }

    int Bson::clone_immutable(lua_State* L)
    {
        int top = lua_gettop(L);
        try
        {
            if (top == 1)
            {
                std::string tmp(lua::as_string(L, -1));
                lua_pop(L, 1);
                Lunar<lua::Bson_ro>::push(L,
                        new Bson_ro(node_->nav(tmp)),
                        true);
            }
            else
            {
                Lunar<lua::Bson_ro>::push(L,
                        new Bson_ro(*node_),
                        true);
            }
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 1;
    }

    int Bson::set_null(lua_State* L)
    {
        std::string tmp(lua::as_string(L, -1));
        try
        {
            node_->set_child(tmp, lj::bson::new_null());
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Bson::set_document(lua_State* L)
    {
        int top = lua_gettop(L);
        std::string tmp(lua::as_string(L, 1));
        try
        {
            if (top == 2)
            {
                Bson* val = Lunar<Bson>::check(L, 2);
                node_->set_child(tmp,
                        new lj::bson::Node(val->node()));
            }
            else
            {
                node_->set_child(tmp, new lj::bson::Node());
            }
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        lua_pop(L, top);
        return 0;
    }

    int Bson::set_array(lua_State* L)
    {
        int top = lua_gettop(L);
        std::string tmp(lua::as_string(L, -1));
        try
        {
            if (top == 2)
            {
                Bson* val = Lunar<Bson>::check(L, 2);
                node_->set_child(tmp,
                        new lj::bson::Node(val->node()));
            }
            else
            {
                node_->set_child(tmp, lj::bson::new_array());
            }
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        lua_pop(L, top);
        return 0;
    }

    int Bson::set_boolean(lua_State* L)
    {
        std::string tmp(lua::as_string(L, -2));
        try
        {
            node_->set_child(tmp,
                    lj::bson::new_boolean(lua_toboolean(L, -1)));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Bson::set_string(lua_State* L)
    {
        std::string tmp(lua::as_string(L, -2));
        try
        {
            node_->set_child(tmp,
                    lj::bson::new_string(lua::as_string(L, -1)));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Bson::set_int32(lua_State* L)
    {
        std::string tmp(lua::as_string(L, -2));
        try
        {
            node_->set_child(tmp,
                    lj::bson::new_int32(lua_tointeger(L, -1)));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Bson::set_int64(lua_State* L)
    {
        std::string tmp(lua::as_string(L, -2));
        try
        {
            node_->set_child(tmp,
                    lj::bson::new_int64(lua_tointeger(L, -1)));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }
    
    int Bson::set_uuid(lua_State* L)
    {
        std::string tmp(lua::as_string(L, -2));
        try
        {
            Uuid* val = Lunar<Uuid>::check(L, -1);
            node_->set_child(tmp,
                    lj::bson::new_uuid(val->id()));
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 0;
    }

    int Bson::as_string(lua_State* L)
    {
        std::string tmp(lj::bson::as_string(*node_));
        lua_pushstring(L, tmp.c_str());
        return 1;
    }

    int Bson::as_nil(lua_State* L)
    {
        lua_pushnil(L);
        return 1;
    }

    int Bson::as_table(lua_State* L)
    {
        if (lj::bson::Type::k_document == node_->type())
        {
            std::map<std::string, lj::bson::Node*> tmp =
                    node_->to_map();
            lua_createtable(L, 0, tmp.size());
            int table = lua_gettop(L);
            for (auto iter = tmp.begin();
                    tmp.end() != iter;
                    ++iter)
            {
                lua_pushstring(L, (*iter).first.c_str());
                Lunar<Bson_ro>::push(L,
                        new Bson_ro(*((*iter).second)),
                        true);
                lua_rawset(L, table);
            }
        }
        else if (lj::bson::Type::k_array == node_->type())
        {
            std::vector<lj::bson::Node*> tmp = 
                    node_->to_vector();
            lua_createtable(L, tmp.size(), 0);
            int table = lua_gettop(L);
            int i = 1;
            for (auto iter = tmp.begin();
                    tmp.end() != iter;
                    ++iter, ++i)
            {
                Lunar<Bson_ro>::push(L,
                        new Bson_ro(**iter),
                        true);
                lua_rawseti(L, table, i);
            }
        }
        else
        {
            lua_newtable(L);
        }
        return 1;
    }

    int Bson::as_number(lua_State* L)
    {
        int64_t tmp = lj::bson::as_int64(*node_);
        lua_pushinteger(L, tmp);
        return 1;
    }

    int Bson::as_boolean(lua_State* L)
    {
        bool tmp = lj::bson::as_boolean(*node_);
        lua_pushboolean(L, tmp);
        return 1;
    }

    int Bson::__tostring(lua_State* L)
    {
        std::string tmp(lj::bson::as_pretty_json(*node_));
        lua_pushstring(L, tmp.c_str());
        return 1;
    }

    int Bson::__index(lua_State* L)
    {
        return path(L);
    }

    int Bson::as_uuid(lua_State* L)
    {
        Lunar<Uuid>::push(L, new Uuid(lj::bson::as_uuid(*node_)), true);
        return 1;
    }

    const char Bson_ro::LUNAR_CLASS_NAME[] = "Bson_ro";
    Lunar<Bson_ro>::RegType Bson_ro::LUNAR_METHODS[] =
    {
        LUNAR_METHOD(Bson, type)
        ,LUNAR_METHOD(Bson_ro, path)
        ,LUNAR_METHOD(Bson, clone)
        ,LUNAR_METHOD(Bson, as_string)
        ,LUNAR_METHOD(Bson, as_nil)
        ,LUNAR_METHOD(Bson, as_table)
        ,LUNAR_METHOD(Bson, as_number)
        ,LUNAR_METHOD(Bson, as_boolean)
        ,LUNAR_METHOD(Bson, as_uuid)
        ,LUNAR_METHOD(Bson, __tostring)
        ,LUNAR_METHOD(Bson, __index)
        ,{0, 0}
    };
    Bson_ro::Bson_ro(lua_State* L) :
            Bson(L)
    {
    }

    Bson_ro::Bson_ro(const lj::bson::Node& val) :
            Bson(val)
    {
    }

    Bson_ro::~Bson_ro()
    {
    }

    int Bson_ro::path(lua_State* L)
    {
        const lj::bson::Node& cnode(node());
        std::string tmp(lua::as_string(L, -1));
        try
        {
            Lunar<lua::Bson_ro>::push(L, new Bson_ro(cnode.nav(tmp)), true);
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 1;
    }

    int Bson_ro::clone(lua_State* L)
    {
        int top = lua_gettop(L);
        try
        {
            const lj::bson::Node& cnode(node());
            if (top == 1)
            {
                std::string tmp(lua::as_string(L, -1));
                Lunar<lua::Bson_ro>::push(L, new Bson_ro(cnode.nav(tmp)), true);
            }
            else
            {
                Lunar<lua::Bson_ro>::push(L, new Bson_ro(cnode), true);
            }
        }
        catch (lj::Exception& ex)
        {
            lua_pushstring(L, ex.str().c_str());
            lua_error(L);
        }
        return 1;
    }

}; // namespace lua
