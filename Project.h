#pragma once
/*
 \file Project.h
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

#include <list>
#include "Model.h"
#include "lunar.h"

//! Project Class.
/*!
 \author Jason Watson
 \version 1.0
 \date July 3, 2009
 */
class Project : public Model<Project> {
public:
    //! Lua bindings classname.
    static const char LUNAR_CLASS_NAME[];
    
    //! Lua Bindings method array.
    static Lunar<Project>::RegType LUNAR_METHODS[];
    
    //! Get a list of all projects.
    /*!
     \par Projects in the list must be deallocated with "delete".
     \return A list of all projects.
     */
    static std::list<Project *> all();
    
    //! Get a list of projects matching a search term.
    /*!
     \par Projects in the list must be deallocated with "delete".
     \param term The term to search for.
     \return A list of matching Projects.
     */
    static std::list<Project *> like(const std::string &term);
    
    //! Get a Project by name.
    /*!
     \param name The project name.
     \param model Pointer to the object to populate.
     \exception std::string When the name is unknown.
     \exception tokyo::Exception When the database cannot be read.
     */
    static void at_name(const std::string &name, Project *model);
    
    //! Get a project by id.
    /*!
     \param key The project primary key.
     \param model Pointer to the object to populate.
     \exception tokyo::Exception When the database cannot be read or the record
     cannot be found.
     */
    static void at(const unsigned long long key, Project *model);
    
    //! Create a new project object.
    Project();
    
    //! Load a project object by key.
    /*!
     \param key The primary key.
     */
    Project(unsigned long long key);
    
    //! Load a project object by name.
    /*!
     \param name The name of the project.
     */
    Project(const std::string &name);
    
    //! Lua constructor.
    /*!
     \param L Pointer to a lua state.
     */
    Project(lua_State *L);
    
    //! Delete the project object.
    virtual ~Project();
    
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
    
    //! Get the commit feed URL.
    /*!
     \return The URL of the commit feed. Used to create dashboard widgets.
     */
    std::string commit_feed() const { return _commit_feed; };
    
    //! Set the commit feed URL.
    /*!
     \param feed The commit feed URL.
     */
    void commit_feed(const std::string &feed) { _commit_feed = feed; };
    
    //! List of versions.
    /*!
     \return A reference to the versions list.
     */
    std::list<std::string> &versions() { return _versions; };
    
    //! List of versions.
    /*!
     \return A copy of the versions list.
     */
    std::list<std::string> versions() const { return _versions; };
    
    //! List of categories.
    /*!
     \return A reference to the categories list.
     */
    std::list<std::string> &categories() { return _categories; };
    
    //! List of categories.
    /*!
     \return A copy of the categories list.
     */
    std::list<std::string> categories() const { return _categories; };
    
    virtual const std::string serialize() const;
    virtual void populate(OpenProp::File *props);
protected:
    virtual ModelDB<Project> *dao() const;
private:
    std::string _name;
    std::string _commit_feed;
    std::list<std::string> _versions;
    std::list<std::string> _categories;
};
