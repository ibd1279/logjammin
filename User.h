#pragma once
/*
 \file User.h
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
#include <set>
#include "Model.h"
#include "lunar.h"
#include "Role.h"

namespace logjammin {
    
    //! User Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 29, 2009
     */
    class User : public Model {
    public:
        //=====================================================================
        // User Lua Integration
        //=====================================================================
        
        //! Lua bindings classname.
        static const char LUNAR_CLASS_NAME[];
        
        //! Lua Bindings method array.
        static Lunar<User>::RegType LUNAR_METHODS[];
        
        //=====================================================================
        // User Static Methods
        //=====================================================================
        
        //! Get a list of all users.
        /*!
         \par
         Users in the list must be deallocated with "delete".
         \return A list of all users.
         */
        static void all(std::list<User *> &results);
        
        //! Get a list of users matching a specific term.
        /*!
         \par
         The email address and name are searched.
         \par
         Users in the list must be deallocated with "delete".
         \param term The search term.
         \return A list of matching users.
         */
        static void like(const std::string &term,
                         std::list<User *> &results);
        
        //! Get a user by primary key.
        /*!
         \param key The primary key.
         \param model Pointer to the object to populate.
         */
        static void at(unsigned long long key,
                       User &model);
        
        //! Get a user by login.
        /*!
         \param login The login to search for.
         \param model Pointer to the object to populate.
         */
        static void at_login(const std::string &login,
                             User &model);
        
        //=====================================================================
        // User ctor/dtor
        //=====================================================================
        
        //! Create a user object.
        User() : Model(), _cached_allowed(NULL) {}
        
        //! Create a copy of a user object.
        User(const User &orig) : Model(orig), _cached_allowed(NULL) {}
        
        //! Create a user object by document.
        User(const tokyo::Document &d) : Model(d), _cached_allowed(NULL) {}
        
        //! Create a user object in a lua context.
        User(lua_State *L) : Model(), _cached_allowed(NULL) {}
        
        //! Delete the user object.
        virtual ~User();
        
        //=====================================================================
        // User Instance
        //=====================================================================
        
        //! Increment the login count for the user.
        void incr_login_count() { field("/login_count", field("/login_count").to_long() + 1); }
        
        //! Get the role for this user.
        Role role() const {
            Role r;
            Role::at(field("/role").to_long(), r);
            return r;
        };
        
        //! Set the role for this user.
        void role(const Role &r);
        
        void cookie(const std::string &cookie);
        
        //! Checks that the provided string matches the stored cookie.
        virtual bool check_cookie(const std::string &cookie);
        
        //! Check a user is allowed to invoke an action.
        virtual bool check_allowed(const std::string &action);
    protected:
        virtual tokyo::Storage *dao() const;
        std::set<std::string> *_cached_allowed;
    };
}; // namespace logjammin
