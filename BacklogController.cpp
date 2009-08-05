/*
 \file BacklogController.cpp
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

#include "BacklogController.h"
#include "Project.h"
#include "Backlog.h"

bool BacklogListController::is_requested(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    if(!request->has_attribute("authenticated"))
        return false;
    if(request->has_attribute("handled"))
        return false;
    
    if(args.size() < 2)
        return false;
    return (args.back().compare("backlog-list") == 0);
}

void BacklogListController::execute(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    // Remove the command name.
    args.pop_back();
    
    Project project;
    if(args.size() > 0) {
        std::string project_key = args.front();
        Project::at(atol(project_key.c_str()), &project);
        request->attribute("project", project_key);
        args.pop_front();
    }
    
    std::string version;
    if(args.size() > 0) {
        version = args.front();
        request->attribute("version", version);
        args.pop_front();
    }
    
    std::string category;
    if(args.size() > 0) {
        category = args.front();
        request->attribute("category", category);
        args.pop_front();
    }
    
    try {
        request->context_object_list("backlogs",
                                     Backlog::all(project,
                                                  version,
                                                  category,
                                                  request->param("disposition-above"),
                                                  request->param("disposition-below")),
                                     true);
    } catch(const std::string &ex) {
        request->attribute("_error", ex);
    } catch(tokyo::Exception &ex) {
        request->attribute("_error", ex.msg);
    }
    
    if(request->header("HTTP_X_REQUESTED_WITH").compare("XMLHttpRequest") == 0)
        response->execute("backlog-list.json", request);
    else
        response->execute("backlog-list.html", request);
    request->attribute("handled", "true");
}

bool BacklogEditController::is_requested(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    if(!request->has_attribute("authenticated"))
        return false;
    if(request->has_attribute("handled"))
        return false;
    
    if(args.size() < 1)
        return false;
    return (args.back().compare("backlog-edit") == 0);
}

void BacklogEditController::execute(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    // Remove the command name.
    args.pop_back();
    
    User *user = request->context_object<User>("_user");
    Backlog b;
    if(args.size() > 0)
        Backlog::at(atol(args.front().c_str()), &b);
    else {
        Project::at(atol(request->param("project").c_str()), &(b.project()));
        b.version(request->param("version"));
        b.category(request->param("category"));
    }
    
    if(request->is_post()) {
        Project::at(atol(request->param("project").c_str()), &(b.project()));
        b.version(request->param("version"));
        b.category(request->param("category"));
        b.story(request->param("story"));
        b.disposition(request->param("disposition"));
        b.estimate(atof(request->param("estimate").c_str()));
        b.actual(atof(request->param("actual").c_str()));

        // Store the comment.
        if(request->param("comments").size() > 0) {
            std::string comment(user->name());
            comment.append(": ").append(request->param("comments"));
            b.comments().push_back(comment);
        }
        
        // Store the tags.
        std::pair<CGI::Request::param_map::const_iterator, CGI::Request::param_map::const_iterator> range;
        range = request->params().equal_range("tags");
        b.tags().clear();
        for(CGI::Request::param_map::const_iterator iter = range.first;
            iter != range.second;
            ++iter) {
            if(iter->second.size()) b.tags().insert(iter->second);
        }
        
        // Store the assignments
        range = request->params().equal_range("assigned");
        for(CGI::Request::param_map::const_iterator iter = range.first;
            iter != range.second;
            ++iter) {
            if(iter->second.size()) {
                User assigned_user(atol(iter->second.c_str()));
                std::ostringstream user_pkey;
                user_pkey << "assigned:" << assigned_user.pkey();
                b.tags().insert(user_pkey.str());
                
                std::ostringstream user_name;
                user_name << "assigned:" << assigned_user.name();
                b.tags().insert(user_name.str());
            }
        }
        
        // Attempt to save.
        try {
            b.save();
            
            // On success, redirect.
            std::ostringstream url;
            url << request->original_request_script();
            url << "/" << b.project().pkey() << "/" << b.version() << "/backlog-list?_msg=SAVE_SUCCESS";
            response->redirect(url.str());
        } catch(const std::string &ex) {
            request->attribute("_error", ex);
        } catch(tokyo::Exception &ex) {
            request->attribute("_error", ex.msg);
        }
    }
    
    request->context_object("backlog", &b, false);
    request->context_object_list("projects", Project::all(), true);
    request->context_object_list("users", User::all(), true);
    response->execute("backlog-edit.html", request);
    request->attribute("handled", "true");
}

bool BacklogSearchController::is_requested(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    if(!request->has_attribute("authenticated"))
        return false;
    if(request->has_attribute("handled"))
        return false;
    
    if(args.size() != 2)
        return false;
    return (args.back().compare("backlog-search") == 0);
}

void BacklogSearchController::execute(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    // Remove the command name.
    args.pop_back();
    
    Project project;
    if(args.size() > 0) {
        std::string project_key = args.front();
        Project::at(atol(project_key.c_str()), &project);
        request->attribute("project", project_key);
        args.pop_front();
    }
    
    std::string version;
    if(args.size() > 0) {
        version = args.front();
        request->attribute("version", version);
        args.pop_front();
    }
    
    std::string category;
    if(args.size() > 0) {
        category = args.front();
        request->attribute("category", category);
        args.pop_front();
    }
    
    try {
        request->context_object_list("backlogs",
                                     Backlog::like(request->param("q"), 
                                                   project,
                                                   version,
                                                   category,
                                                   request->param("disposition-above"),
                                                   request->param("disposition-below")),
                                     true);
    } catch(const std::string &ex) {
        request->attribute("_error", ex);
    } catch(tokyo::Exception &ex) {
        request->attribute("_error", ex.msg);
    }
    
    response->execute("backlog-list.html", request);
    request->attribute("handled", "true");
}


