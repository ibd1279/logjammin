#pragma once
/*!
 \file lua/Document.h
 \brief lua document header.
 \author Jason Watson

 Copyright (c) 2014, Jason Watson
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

#include "lj/Document.h"
#include "lua/lunar.h"

namespace lua
{
    //! Lua bridge for Document objects.
    /*!
     \par
     Provides a representation of Document objects inside Lua.
     */
    class Document
    {
    private:
        lj::Document* doc_; //!< Pointer to the real Document.
        bool gc_; //!< Memory manager switch.
    public:
        static const char LUNAR_CLASS_NAME[]; //!< Table name for Lua.
        static Lunar<Document>::RegType LUNAR_METHODS[]; //!< Array of methods to register in Lua.

        //! Create a new lua Document object.
        /*!
         A new, empty document is created for this object.
         \param L The lua state.
         */
        Document(lua_State* L);

        //! Create a lua Document around an existing document.
        /*!
         \param val The document to be wrapped in a lua object.
         \param gc True if lua should delete the \c val pointer. False otherwise.
         */
        Document(lj::Document* val, bool gc = false);

        //! Destructor.
        ~Document();
        int parent(lua_State* L);
        int vclock(lua_State* L);
        int version(lua_State* L);
        int key(lua_State* L);
        int id(lua_State* L);
        int suppress(lua_State* L);
        int dirty(lua_State* L);
        int get(lua_State* L);
        int exists(lua_State* L);
        int wash(lua_State* L);
        int rekey(lua_State* L);
        int branch(lua_State* L);
        int set(lua_State* L);
        int push(lua_State* L);
        int increment(lua_State* L);
        int encrypt(lua_State* L);
        int decrypt(lua_State* L);
        int __tostring(lua_State* L);
        int __index(lua_State* L);
    };
}; // namespace lua
