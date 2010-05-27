#pragma once
/*
 \file Role.h
 \author Jason Watson
 Copyright (c) 2009, Jason Watson
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

#include <string>
#include <list>
#include "lunar.h"
#include "lj/Bson.h"

namespace logjammin {
    
    //! Role Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 9, 2009
     */
    class Role {
    public:
        //! Class name in lua.
        static const char LUNAR_CLASS_NAME[];
        
        //! Table of methods for lua.
        static Lunar<Role>::RegType LUNAR_METHODS[];
        
        //! Create a new role object.
        Role(lj::Bson* ptr);

        //! Delete the role.
        ~Role();
        
        //! Get a field.
        /*!
         \param L The lua state.
         \return 1
         */
        int __index(lua_State* L);
        
        //! Get the name.
        /*!
         \return the name.
         */
        std::string name();
        
        //! Set the name.
        /*!
         \param v The new name.
         */
        void name(const std::string& v);
        
        //! Get allowed actions.
        /*!
         \return action set.
         */
        std::set<std::string> allowed();
        
        //! Add an allowed action.
        /*!
         \param L The lua state.
         \return 0
         */
        void add_allowed(const std::string& action);
        
        //! Remove an allowed action.
        /*!
         \param L The lua state.
         \return 0
         */
        void remove_allowed(const std::string& action);
    private:
        lj::Bson* doc_;
    };
}; // namespace logjammin
