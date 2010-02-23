#pragma once
/*
 \file Release.h
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
#include "lunar.h"
#include "Project.h"
#include "Backlog.h"

namespace logjammin {
    
    //! Release Item class.
    /*!
     \author Jason Watson
     \version 1.0
     \date January 30, 2010
     */
    class Release : public Model<Release> {
    public:
        //! Lua bindings classname.
        static const char LUNAR_CLASS_NAME[];
        
        //! Lua Bindings method array.
        static Lunar<Release>::RegType LUNAR_METHODS[];
        
        /*******************************************************************
         * Static methods.
         ******************************************************************/
        
        static std::list<Release *> all(const Project &project,
                                        const std::string &version);
        
        static std::list<Release *> like(const std::string &term,
                                         const Project &project,
                                         const std::string &version);
        
        static void at(const unsigned long long key, Release *model);
        
        /*******************************************************************
         * ctor's and dtor's
         ******************************************************************/
        
        Release();
        
        Release(unsigned long long key);
        
        Release(lua_State *L);
        
        Release(const Release &orig);
        
        virtual ~Release();
        
        /*******************************************************************
         * instance methods.
         ******************************************************************/
        
        //! Get the name.
        std::string name() const { return _name; };
        
        //! Set the name.
        void name(const std::string &n) { _name = n; };
        
        //! Get the version.
        const std::string &version() const { return _version; };
        
        //! Set the version.
        void version(const std::string &v) { _version = v; };
        
        //! Get the constant project.
        const Project &project() const { return _project; };
        
        //! Get the Project.
        Project &project() { return _project; };
        
        //! Set the project.
        void project(const Project &p) { _project = p; };
        
        //! Get a constant list of tasks.
        const std::list<Backlog> &tasks() const { return _tasks; };
        
        //! Get a list of tasks.
        std::list<Backlog> &tasks() { return _tasks; };
        
        //! get the natural key.
        std::string natural_key() const;

        virtual const std::string serialize() const;
        virtual void populate(OpenProp::File *props);
    protected:
        virtual ModelDB<Release> *dao() const;
    private:
        std::string _name, _version;
        Project _project;
        std::list<Backlog> _tasks;
    };
};