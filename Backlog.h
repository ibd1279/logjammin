#pragma once
/*
 \file Backlog.cpp
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

#include <set>
#include <list>
#include "Model.h"
#include "lunar.h"
#include "User.h"
#include "Project.h"

namespace logjammin {
    
    //! BacklogComment class.
    /*!
     \author Jason Watson
     \version 1.0
     \date August 7, 2009
     */
    class BacklogComment {
    public:
        //! Create a new Backlog comment from a property record.
        BacklogComment(OpenProp::Element *props);
        
        //! Copy constructor.
        BacklogComment(const BacklogComment &orig);
        
        //! Create a new BacklogComment.
        BacklogComment(const std::string &comment, const User &user, bool historical);
        
        //! Destructor.
        ~BacklogComment();
        
        //! Get the comment body.
        std::string comment() const { return _comment; };
        
        //! Set the comment body.
        void comment(const std::string &comment) { _comment = comment; };
        
        //! Get a constant reference to the user.
        const User &user() const { return _user; };
        
        //! Get a reference to the user.
        User &user() { return _user; };
        
        //! Set the user for this comment.
        void user(const User &user) { _user = user; };
        
        //! Get the time of the comment.
        long long time() const { return _time; };
        
        //! Get the time of the comment in human readable format.
        std::string time_string() const;
        
        //! Set the time of the comment.
        void time(const long long time) { _time = time; };
        
        //! Get if this is a historical comment.
        bool historical() const { return _historical; };
        
        //! Set if this is a historical comment.
        void historical(const bool historical) { _historical = historical; };
    private:
        std::string _comment;
        User _user;
        long long _time;
        bool _historical;
    };
    
    //! Backlog Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    class Backlog : public Model<Backlog> {
    public:
        //! Lua bindings classname.
        static const char LUNAR_CLASS_NAME[];
        
        //! Lua bindings method array.
        static Lunar<Backlog>::RegType LUNAR_METHODS[];
        
        /*******************************************************************
         * Static methods.
         ******************************************************************/
        
        //! Get all backlogs based on a natural key.
        /*!
         \par
         Backlogs in the list must be deallocated with "delete".
         \param project The Project to search under.
         \param version The version to get the backlogs for.
         \param category The category to get the backlogs for.
         \param lower_disposition The lower disposition to include in results (inclusive).
         \param upper_disposition The upper disposition to include in results (exclusive).
         \return A list of backlogs
         */
        static std::list<Backlog *> all(const Project &project,
                                        const std::string &version,
                                        const std::string &category,
                                        const std::string &lower_disposition,
                                        const std::string &upper_disposition);
        
        //! Get a list of backlogs matching the provided search term.
        /*!
         \par
         Backlogs in the list must be deallocated with "delete".
         \param term The term to search for.
         \param project The Project to search under.
         \param version The version to search under.
         \param category The category to search under.
         \param lower_disposition The lower disposition to include in results (inclusive).
         \param upper_disposition The upper disposition to include in results (exclusive).
         \return A list of backlogs
         */
        static std::list<Backlog *> like(const std::string &term,
                                         const Project &project,
                                         const std::string &version,
                                         const std::string &category,
                                         const std::string &lower_disposition,
                                         const std::string &upper_disposition);
        
        //! load a backlog object by primary key.
        /*!
         \param key The primary key.
         \param model Pointer to the object to populate.
         */
        static void at(const unsigned long long key, Backlog *model);
        
        /*******************************************************************
         * ctor's and dtor's
         ******************************************************************/
        
        //! Create a new Backlog object.
        Backlog();
        
        //! Load a backlog object by key.
        /*!
         \param key The primary key.
         */
        Backlog(unsigned long long key);
        
        //! Lua constructor.
        /*!
         \param L Pointer to a lua state.
         */    
        Backlog(lua_State *L);
        
        //! Delete the backlog object.
        virtual ~Backlog();
        
        /*******************************************************************
         * instance methods.
         ******************************************************************/
        
        //! Get the backlog entry name.
        /*!
         \return The name.
         */
        std::string brief() const { return _brief; };
        
        //! Get a reference to the project associated with this task.
        /*!
         \return A reference to the project associated with this task.
         */
        Project &project() { return _project; };
        
        //! Get a copy of the project associated with this task.
        /*!
         \return A copy of the project associated with this task.
         */
        const Project &project() const { return _project; };
        
        //! Set the project associated with this task.
        /*!
         \param project The project.
         */
        void project(const Project &project) { _project = project; };
        
        //! Get a copy of the version associated with this task.
        /*!
         \return A copy of the version associated with this task.
         */
        std::string version() const { return _version; };
        
        //! Set the version associated with this task.
        /*!
         \param v The version.
         */
        void version(const std::string &v) { _version = v; };
        
        //! Get a copy of the category associated with this task.
        /*!
         \return A copy of the category associated with this task.
         */
        std::string category() const { return _category; };
        
        //! Set the category associated with this task.
        /*!
         \param c The category.
         */
        void category(const std::string &c) { _category = c; };
        
        //! Get a copy of the story text associated with this task.
        /*!
         \return A copy of the story text associated with this task.
         */
        std::string story() const { return _story; };
        
        //! Set the story text associated with this task.
        /*!
         \param s The story text.
         */
        void story(const std::string &s);
        
        //! Get a copy of the disposition of this task.
        /*!
         \return A copy of the disposition of this task.
         */
        std::string disposition() const { return _disposition; };
        
        //! Set the disposition of this task
        /*!
         \param s The disposition.
         */
        void disposition(const std::string &s) { _disposition = s; };
        
        //! Get the estimated effort for this task.
        /*!
         \par
         The estimate is in hours.
         \return The estimate.
         */
        double estimate() const { return _estimate; };
        
        //! Set the estimate for this task.
        /*!
         \par
         The estimate is in hours.
         \param e The estimate.
         */
        void estimate(const double e) { _estimate = e; };
        
        //! Get the actual effort for this task.
        /*!
         \par
         The actual effort is in hours.
         \return The actual effort.
         */
        double actual() const { return _actual; };
        
        //! Set the actual effort for this task.
        /*!
         \par
         The actual effort is in hours.
         \param e The actual effort.
         */
        void actual(const double e) { _actual = e; };
        
        //! Get the natural key for this backlog.
        /*!
         \par
         The natural key consists of the project, the version, the category, and the
         first sentence of the story.
         \return A copy of the natural key.
         */
        std::string natural_key() const;
        
        //! Get a reference to the comments list.
        /*!
         \par
         New comments should be put at the end of the list.
         \return A reference to the comments list.
         */
        std::list<std::string> &comments() { return _comments; };
        
        //! Get a constant reference to the comments list.
        /*!
         \return A constant reference to the comments list.
         */
        const std::list<std::string> &comments() const { return _comments; };
        
        //! Get a reference to the tags set.
        /*!
         \par
         Internal tags start with an underscore.
         \return A reference to the tags set.
         */
        std::set<std::string> &tags() { return _tags; };
        
        //! Get a constant reference to the tags set.
        /*!
         \par Internal tags start with an underscore.
         \return A constant reference to the tags set.
         */
        const std::set<std::string> &tags() const { return _tags; };
        
        virtual const std::string serialize() const;
        virtual void populate(OpenProp::File *props);
    protected:
        virtual ModelDB<Backlog> *dao() const;
    private:
        std::string _brief, _version, _category, _story, _disposition;
        Project _project;
        std::list<std::string> _comments;
        std::set<std::string> _tags;
        double _estimate, _actual;
    };
}; // namespace logjammin
