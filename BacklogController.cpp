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
                                     Backlog::all(project, version, category),
                                     true);
    } catch(const std::string &ex) {
        request->attribute("_error", ex);
    } catch(tokyo::Exception &ex) {
        request->attribute("_error", ex.msg);
    }
    
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

        // Only set the user once.
        if(b.user().pkey() == 0) {
            User::at_login(request->cookie("lj_user_login"), &(b.user()));
        }
        
        // Store the comment.
        if(request->param("comments").size() > 0) {
            std::string comment(request->param("comments"));
            comment.append("\n").append(request->cookie("lj_user_login"));
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

        // Attempt to save.
        try {
            b.save();
            
            // On success, redirect.
            std::ostringstream url;
            url << request->original_request_script();
            url << "/" << b.pkey() << "/backlog-edit?_msg=SAVE_SUCCESS";
            response->redirect(url.str());
        } catch(const std::string &ex) {
            request->attribute("_error", ex);
        } catch(tokyo::Exception &ex) {
            request->attribute("_error", ex.msg);
        }
    }
    
    request->context_object("backlog", &b, false);
    request->context_object_list("projects", Project::all(), true);
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
                                     Backlog::like(request->param("q"), project, version, category),
                                     true);
    } catch(const std::string &ex) {
        request->attribute("_error", ex);
    } catch(tokyo::Exception &ex) {
        request->attribute("_error", ex.msg);
    }
    
    response->execute("backlog-list.html", request);
    request->attribute("handled", "true");
}


