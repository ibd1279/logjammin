#pragma once
/*!
 \file Lua_record_set.h
 \brief Logjamd lj::Record_set wrapper header.
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
#include "lj/lunar.h"
#include "lj/Record_set.h"

#include <string>

namespace logjamd
{
    //! Lua Record_set wrapper.
    /*!
     \par
     Known as "Record_set" in lua.
     \author Jason Watson
     \version 1.0
     \date April 27, 2010
     */
    class Lua_record_set {
    public:
        //! Class name in lua.
        static const char LUNAR_CLASS_NAME[];
        
        //! Table of methods for lua.
        static Lunar<Lua_record_set>::RegType LUNAR_METHODS[];
        
        //! Create a new Lua_record_set wrapper for lj::Record_set.
        /*!
         \par
         Expects a Lua_storage object on top of the stack.
         \param L The lua state.
         */
        Lua_record_set(lua_State* L);
        
        //! Create a new Lua_record_set wrapper for lj::Record_set.
        /*!
         \par
         Takes ownership of the pointer.
         \param filter The lj::Record_set.
         \param cost_data The costs up to this point.
         */
        Lua_record_set(lj::Record_set*filter, lj::Bson* cost_data);
        
        //! Destructor
        ~Lua_record_set();
        
        //! Set the mode of the lj::Record_set to be an intersection.
        /*!
         \par
         Expects an empty stack.
         \par
         Pushes this object back onto the stack.
         \param L The lua state.
         \return 1
         */
        int mode_and(lua_State* L);
        
        //! Set the mode of the lj::Record_set to be an union.
        /*!
         \par
         Expects an empty stack.
         \par
         Pushes this object back onto the stack.
         \param L The lua state.
         \return 1
         */
        int mode_or(lua_State* L);
        
        //! Include an id in the set.
        /*!
         \par
         Expects an integer or function on top of the stack.
         \par
         Pushes a new Lua_record_set object onto the stack.
         \param L The lua state.
         \return 1
         */
        int include(lua_State* L);
        
        //! Exclude an id from the set.
        /*!
         \par
         Expects an integer or function on top of the stack.
         \par
         Pushes new Lua_record_set object onto the stack.
         \param L The lua state.
         \return 1
         */
        int exclude(lua_State* L);
        
        //! Filter the lj::Record_set on an index.
        /*!
         \par
         Pops the value (lua string or Lua_bson) off the stack.
         \par
         Pops the field (lua string) off the stack.
         \par
         Pushes a new Lua_record_set onto the stack.
         \param L The lua state.
         \return 1
         */
        int equal(lua_State* L);
        
        //! Filter the lj::Record_set on an index.
        /*!
         \par
         Pops the value (lua string or Lua_bson) off the stack.
         \par
         Pops the field (lua string) off the stack.
         \par
         Pushes a new Lua_record_set onto the stack.
         \param L The lua state.
         \return 1
         */
        int greater(lua_State* L);
        
        //! Filter the lj::Record_set on an index.
        /*!
         \par
         Pops the value (lua string or Lua_bson) off the stack.
         \par
         Pops the field (lua string) off the stack.
         \par
         Pushes a new Lua_record_set onto the stack.
         \param L The lua state.
         \return 1
         */
        int lesser(lua_State* L);

        //! Filter the lj::Record_set on an index.
        /*!
         \par
         Pops the value (lua string) off the stack.
         \par
         Pops the field (lua string) off the stack.
         \par
         Pushes a new Lua_record_set onto the stack.
         \param L The lua state.
         \return 1
         */
        int contains(lua_State* L);
        
        //! Filter the lj::Record_set on an index.
        /*!
         \par
         Pops the value (lua string) off the stack.
         \par
         Pops the field (lua string) off the stack.
         \par
         Pushes a new Lua_record_set onto the stack.
         \param L The lua state.
         \return 1
         */
        int tagged(lua_State* L);

        //! Get the list of Lua_bson records.
        /*!
         \par
         Expects an empty stack.
         \par
         Pushes a table containing numerical keys, starting at one. Values are
         the Lua_bson objects.
         \param L The lua state.
         \return 1
         */
        int records(lua_State* L);
                
        //! Get the first Lua_bson record.
        /*!
         \par
         Expects an empty stack.
         \par
         Pushes a Lua_bson object.
         \param L The lua state.
         \return 1
         */
        int first(lua_State* L);
        
        //! Get the number of records in this set.
        /*!
         \par
         Expects an empty stack.
         \par
         Pushes the size.
         \param L The lua state.
         \return 1
         */
        int size(lua_State* L);
        
        //! Get the real lj::Record_set object.
        /*!
         \return The real record set object.
         */
        inline lj::Record_set &real_set() { return *filter_; }
        
        //! Get the cost structure associated with this Record_set.
        /*!
         \return The costs.
         */
        inline const lj::Bson& costs() { return *costs_; }
    private:
        lj::Record_set* filter_;
        std::string command_;
        lj::Bson* costs_;
    };
}; // namespace logjamd