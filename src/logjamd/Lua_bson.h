#pragma once
/*!
 \file logjamd_lua.h
 \brief Logjamd lj::Bson wrapper header
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

#include "lj/lunar.h"
#include "lj/Bson.h"

namespace logjamd
{
    //! Lua Bson wrapper.
    /*!
     \par
     Known as "Bson" in lua.
     \author Jason Watson
     \version 1.0
     \date April 26, 2010
     */
    class Lua_bson {
    public:
        //! Class name in lua.
        static const char LUNAR_CLASS_NAME[];
        
        //! Table of methods for lua.
        static Lunar<Lua_bson>::RegType LUNAR_METHODS[];
        
        //! Create a new Lua_bson wrapper for lj::Bson.
        /*!
         \par
         Expects a Lua_bson object or a nothing to be on top of the stack.
         \param L The lua state.
         */
        Lua_bson(lua_State* L);
        
        //! Create a new Lua_bson wrapper for lj::Bson.
        /*!
         \param ptr Object to wrap.
         \param gc True if the wrapper should delete the referenced object.
         */
        Lua_bson(lj::Bson* ptr, bool gc);

        //! Create a new Lua_bson child.
        Lua_bson(lj::Bson* root, const std::string& path, bool gc);
        
        //! Destructor.
        ~Lua_bson();
        
        //! Navigate to a specific lj::Bson object.
        /*!
         \par
         Expects a string to be ontop of the stack.
         \param L The lua state.
         \return 1
         */
        int nav(lua_State* L);
        
        //! Set the value of a lj::Bson object.
        /*!
         \par
         Expects any type of value ontop of the stack.
         \param L the lua state.
         \return 0
         */
        int set(lua_State* L);
        
        //! Push a child of a lj::Bson object.
        /*!
         \par
         Expects any type to be ontop of the stack.
         \param L The lua state.
         \return 0
         */
        int push(lua_State* L);
        
        //! Get the lj::Bson value as a native lua value.
        /*!
         \par
         Expects an empty stack.
         \par
         Arrays and documents are returned as strings.
         \param L The lua state.
         \return 1
         */
        int get(lua_State* L);
        
        //! Save the lj::Bson object to disk.
        /*!
         \par
         Expects a string to be ontop of the stack.
         \param L The lua state.
         \return 0
         */
        int save(lua_State* L);
        
        //! Load an lj::Bson object from disk.
        /*!
         \par
         Expects a string to be ontop of the stack.
         \param L The lua state.
         \return 1
         */
        int load(lua_State* L);
        
        //! Convert the current object to a string.
        /*!
         \par
         This method is injected into the metatable in lua.
         \par
         Expects an empty stack.
         \param L The lua state.
         \return 1
         */
        int __tostring(lua_State* L);
        
        //! Short cut for calling nav()
        /*!
         \par
         This method is injected into the metatable in lua.
         \par
         Expects a string to be ontop of the stack.
         \todo This needs to be modified to use the metatable instead of the
         global.
         \param L The lua state.
         \return 1
         \sa logjamd::Lua_bson::nav()
         */
        int __index(lua_State* L);
        
        //! Get the lj::Bson object being wrapped.
        /*!
         \return The real object.
         */
        inline lj::Bson &real_node()
        {
            return *node_;
        }
    private:
        void record_delta();
        
        lj::Bson* root_;
        lj::Bson* node_;
        const bool gc_;
        const std::string path_;
    };
    
}; // namespace logjamd