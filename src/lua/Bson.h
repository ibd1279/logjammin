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

namespace lua
{
    //! Lua bridge for Bson objects.
    /*!
     \par
     Provides a representation of Bson objects inside Lua.
     */
    class Bson
    {
    private:
        std::shared_ptr<lj::bson::Node> node_; //!< Pointer to the real Bson node.
    public:
        static const char LUNAR_CLASS_NAME[]; //!< Table name for Lua.
        static Lunar<Bson>::RegType LUNAR_METHODS[]; //!< Array of methods to register in Lua.

        //! Create a new lua Bson object.
        /*!
         A new, empty Node is created for this object.
         \param L The lua state.
         */
        Bson(lua_State* L);

        //! Copy an existing lua Bson object.
        /*!
         \par
         The provided lj::bson::Node is copied and the lua Bson object is
         created around the copy. You will need to use lua::Bson::node() for
         changes to appear in the Lua environment.
         \param val The Bson data to create this Lua representation around.
         */
        Bson(const lj::bson::Node& val);

        //! Create a Lua facade ontop of an existing shared Node.
        /*!
         \par
         This is mainly used to represent navigated paths and children inside
         a Bson document.
         \par
         Because freeing the memory associated with the root
         will cascade and release all the memory associated with the children,
         this constructor pins memory management on the root. while all the
         methods will be performed against the provided path.
         \param root The root of the Bson node.
         \param path The path this node represents from the root.
         */
        Bson(std::shared_ptr<lj::bson::Node>& root,
                const std::string& path);

        //! Destructor.
        virtual ~Bson();

        //! Get the underlying lj::bson::Node.
        /*!
         \par
         This is a C++ helper function.
         \return A reference to the underlying node.
         */
        lj::bson::Node& node();

        //! Get the bson type of the node.
        /*!
         \par Lua Parameters
         None.
         \par Lua Return
         String representing the bson type.
         \param L The lua state.
         \return Number of items returned in lua.
         \sa lj::bson::type_string()
         \sa lj::bson::Node::type()
         */
        int type(lua_State* L);

        //! Nullify the bson node.
        /*!
         \par Lua Parameters
         None.
         \par Lua Return
         None.
         \param L The lua state.
         \return Number of items returned in lua.
         \sa lj::bson::Node::nullify()
         */
        int nullify(lua_State* L);
        virtual int path(lua_State* L);
        virtual int clone(lua_State* L);
        int clone_immutable(lua_State* L);
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

    //! Lua bridge for const Bson objects.
    /*!
     \par
     Provides a representation of Bson objects inside Lua. This specific
     representation lacks any set methods and is only able to navigate to
     existing paths.
     */
    class Bson_ro : public Bson
    {
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<Bson_ro>::RegType LUNAR_METHODS[];
        Bson_ro(lua_State* L);
        Bson_ro(const lj::bson::Node& val);
        virtual ~Bson_ro();
        virtual int path(lua_State* L) override;
        virtual int clone(lua_State* L) override;
    }; // class Bson_ro

}; // namespace lua
