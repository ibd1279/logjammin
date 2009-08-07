/*
 \file ProjectController.cpp
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

#include "ProjectController.h"
#include "Project.h"

namespace logjammin {
    namespace controller {
        
        bool ProjectListController::is_requested(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            if(!request->has_attribute("authenticated"))
                return false;
            if(request->has_attribute("handled"))
                return false;
            
            if(args.size() < 1)
                return false;
            return (args.back().compare("project-list") == 0);
        }
        
        void ProjectListController::execute(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            // Remove the command name.
            args.pop_back();
            
            try {
                request->context_object_list("projects",
                                             Project::all(),
                                             true);
            } catch(const std::string &ex) {
                request->attribute("_error", ex);
            } catch(tokyo::Exception &ex) {
                request->attribute("_error", ex.msg);
            }
            
            response->execute("project-list.html", request);
            request->attribute("handled", "true");
        }
        
        bool ProjectEditController::is_requested(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            if(!request->has_attribute("authenticated"))
                return false;
            if(request->has_attribute("handled"))
                return false;
            
            if(args.size() < 1)
                return false;
            return (args.back().compare("project-edit") == 0);
        }
        
        void ProjectEditController::execute(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            // Remove the command name.
            args.pop_back();
            
            Project p;
            if(args.size() > 0)
                Project::at(atol(args.front().c_str()), &p);
            
            if(request->is_post()) {
                p.name(request->param("name"));
                p.commit_feed(request->param("commit_feed"));
                
                std::pair<CGI::Request::param_map::const_iterator, CGI::Request::param_map::const_iterator> range;
                range = request->params().equal_range("version");
                p.versions().clear();
                for(CGI::Request::param_map::const_iterator iter = range.first;
                    iter != range.second;
                    ++iter) {
                    if(iter->second.size()) p.versions().push_back(iter->second);
                }
                p.versions().sort();
                
                // Categories.
                range = request->params().equal_range("category");
                p.categories().clear();
                for(CGI::Request::param_map::const_iterator iter = range.first;
                    iter != range.second;
                    ++iter) {
                    if(iter->second.size()) p.categories().push_back(iter->second);
                }
                p.categories().sort();
                
                // Attempt to save.
                try {
                    p.save();
                    
                    // On success, redirect.
                    std::ostringstream url;
                    url << request->original_request_script();
                    url << "/" << p.pkey() << "/project-edit?_msg=SAVE_SUCCESS";
                    response->redirect(url.str());
                } catch(const std::string &ex) {
                    request->attribute("_error", ex);
                } catch(tokyo::Exception &ex) {
                    request->attribute("_error", ex.msg);
                }
            }
            
            request->context_object("project", &p, false);
            response->execute("project-edit.html", request);
            request->attribute("handled", "true");
        }
        
        bool ProjectPurgeController::is_requested(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            if(!request->has_attribute("authenticated"))
                return false;
            if(request->has_attribute("handled"))
                return false;
            
            if(args.size() != 2)
                return false;
            return (args.back().compare("project-purge") == 0);
        }
        
        void ProjectPurgeController::execute(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            // Remove the command name.
            args.pop_back();
            
            Project p(atol(args.front().c_str()));
            
            if(request->is_post()) {
                try {
                    p.purge();
                    std::ostringstream url;
                    url << request->original_request_script();
                    url << "/project-list?_msg=PURGE_SUCCESS";
                    response->redirect(url.str());
                } catch(const std::string &ex) {
                    request->attribute("_error", ex);
                } catch(tokyo::Exception &ex) {
                    request->attribute("_error", ex.msg);
                }
            }
            
            request->context_object("project", &p, false);
            response->execute("project-purge.html", request);
            request->attribute("handled", "true");
        }
        
        bool ProjectSearchController::is_requested(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            if(!request->has_attribute("authenticated"))
                return false;
            if(request->has_attribute("handled"))
                return false;
            
            if(args.size() != 1)
                return false;
            return (args.back().compare("project-search") == 0);
        }
        
        void ProjectSearchController::execute(CGI::Request *request, CGI::Response *response) {
            if(request->has_param("q")) {
                try {
                    request->context_object_list("projects",
                                                 Project::like(request->param("q")),
                                                 true);
                } catch(const std::string &ex) {
                    request->attribute("_error", ex);
                } catch(tokyo::Exception &ex) {
                    request->attribute("_error", ex.msg);
                }
            }
            
            response->execute("project-list.html", request);
            request->attribute("handled", "true");
        }
    };
};
