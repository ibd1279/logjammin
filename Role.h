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
#include "Model.h"
#include "lunar.h"

namespace logjammin {
    
    //! Role Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 9, 2009
     */
    class Role : public Model<Role> {
    public:
        //! Lua bindings class name.
        static const char LUNAR_CLASS_NAME[];
        
        //! Luna bindings method array.
        static Lunar<Role>::RegType LUNAR_METHODS[];
        
        /*******************************************************************
         * Static methods.
         ******************************************************************/
        
        //! Get a list of all roles in the database.
        /*!
         \par Roles in the list must be deallocated with "delete".
         \return A list of roles.
         */
        static std::list<Role *> all();
        
        //! Get a role by primary key.
        /*!
         \param key the primary key.
         \param model Pointer to the object to populate.
         \exception tokyo::Exception When the record cannot be found.
         */
        static void at(unsigned long long key, Role *model);
        
        //! Get a role by name.
        /*!
         \param name The role name.
         \param model Pointer to the object to populate.
         \exception tokyo::Exception When the record cannot be read.
         \exception std::string When the record does not exist.
         */
        static void at_name(const std::string &name, Role *model);
        
        /*******************************************************************
         * ctor's and dtor's
         ******************************************************************/
        
        //! Create a new role object.
        Role();
        
        //! Create a copy of a role object.
        /*!
         \param orig The original role object.
         */
        Role(const Role &orig);
        
        //! Create a new role object from a primary key.
        /*!
         \param key The primary key.
         \exception tokyo::Exception When the record cannot be found.
         */
        Role(unsigned long long key);
        
        //! Create a new role object from the name.
        /*!
         \param name The role name.
         \exception tokyo::Exception When the record cannot be read.
         \exception std::string When the record does not exist.
         */
        Role(const std::string &name);
        
        //! Lua constructor.
        /*!
         \param L Pointer to the lua state.
         */
        Role(lua_State *L);
        
        //! Delete the role.
        virtual ~Role();
        
        /*******************************************************************
         * instance methods.
         ******************************************************************/
        
        //! Get the name.
        /*!
         \return The name of the project.
         */
        std::string name() const { return _name; };
        
        //! Set the name.
        /*!
         \param name The name of the project.
         */
        void name(const std::string &name) { _name = name; };
        
        //! List of allowed actions.
        /*!
         \return A reference to the list of allowed actions.
         */
        std::list<std::string> &allowed() { return _allowed; };
        
        //! List of allowed actions.
        /*!
         \return A copy of the list of allowed actions.
         */
        std::list<std::string> allowed() const { return _allowed; };
        
        virtual const std::string serialize() const;
        virtual void populate(OpenProp::File *props);
    protected:
        virtual ModelDB<Role> *dao() const;
    private:
        std::string _name;
        std::list<std::string> _allowed;
    };
}; // namespace logjammin
