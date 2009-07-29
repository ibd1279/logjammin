#include "UserController.h"
#include "Role.h"
#include "User.h"


bool UserListController::is_requested(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    if(!request->has_attribute("authenticated"))
        return false;
    if(request->has_attribute("handled"))
        return false;
    
    if(args.size() < 1)
        return false;
    return (args.back().compare("user-list") == 0);
}

void UserListController::execute(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    // Remove the command name.
    args.pop_back();
    
    try {
        request->context_object_list("users",
                                     User::all(),
                                     true);
    } catch(const std::string &ex) {
        request->attribute("_error", ex);
    } catch(tokyo::Exception &ex) {
        request->attribute("_error", ex.msg);
    }
    
    response->execute("user-list.html", request);
    request->attribute("handled", "true");
}

bool UserEditController::is_requested(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    if(!request->has_attribute("authenticated"))
        return false;
    if(request->has_attribute("handled"))
        return false;
    
    if(args.size() < 1)
        return false;
    return (args.back().compare("user-edit") == 0);
}

void UserEditController::execute(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    // Remove the command name.
    args.pop_back();
    
    User u;
    if(args.size() > 0)
        User::at(atol(args.front().c_str()), &u);
    
    if(request->is_post()) {
        u.name(request->param("name"));
        u.role(Role(atol(request->param("role").c_str())));
        
        std::pair<CGI::Request::param_map::const_iterator, CGI::Request::param_map::const_iterator> range;
        range = request->params().equal_range("login");
        u.logins().clear();
        for(CGI::Request::param_map::const_iterator iter = range.first;
            iter != range.second;
            ++iter) {
            if(iter->second.size()) u.logins().push_back(iter->second);
        }
        u.logins().sort();
        
        // Attempt to save.
        try {
            u.save();
            
            // On success, redirect.
            std::ostringstream url;
            url << request->original_request_script();
            url << "/user-list?_msg=SAVE_SUCCESS";
            response->redirect(url.str());
        } catch(const std::string &ex) {
            request->attribute("_error", ex);
        } catch(tokyo::Exception &ex) {
            request->attribute("_error", ex.msg);
        }
    }
    
    request->context_object("user", &u, false);
    request->context_object_list("roles", Role::all(), true);
    response->execute("user-edit.html", request);
    request->attribute("handled", "true");
}

bool UserPurgeController::is_requested(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    if(!request->has_attribute("authenticated"))
        return false;
    if(request->has_attribute("handled"))
        return false;
    
    if(args.size() != 2)
        return false;
    return (args.back().compare("user-purge") == 0);
}

void UserPurgeController::execute(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    // Remove the command name.
    args.pop_back();
    
    User u(atol(args.front().c_str()));
    
    if(request->is_post()) {
        try {
            u.purge();
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
    
    request->context_object("user", &u, false);
    response->execute("user-purge.html", request);
    request->attribute("handled", "true");
}

bool UserSearchController::is_requested(CGI::Request *request, CGI::Response *response) {
    std::list<std::string> args(request->split_path_info());
    
    if(!request->has_attribute("authenticated"))
        return false;
    if(request->has_attribute("handled"))
        return false;
    
    if(args.size() != 1)
        return false;
    return (args.back().compare("user-search") == 0);
}

void UserSearchController::execute(CGI::Request *request, CGI::Response *response) {
    if(request->has_param("q")) {
        try {
            request->context_object_list("users",
                                         User::like(request->param("q")),
                                         true);
        } catch(const std::string &ex) {
            request->attribute("_error", ex);
        } catch(tokyo::Exception &ex) {
            request->attribute("_error", ex.msg);
        }
    }
    
    response->execute("user-list.html", request);
    request->attribute("handled", "true");
}


