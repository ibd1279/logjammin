#include <stdlib.h>
#include <time.h>
#include "Controller.h"
#include "User.h"
#include "OpenID.h"
#include "OpenIDConsumer.h"
#include <iostream>

bool AuthenticateFilter::is_requested(CGI::Request *request, CGI::Response *response) {
    srand(time(NULL));
    return true;
}

void AuthenticateFilter::execute(CGI::Request *request, CGI::Response *response) {
    if(!request->has_param("logout")) {
        
        // User isn't logging out, check the cookies first.
        if(request->has_cookie("lj_user_login") && request->has_cookie("lj_user_cookie")) {
            try {
                // Load the user.
                User *user = new User(request->cookie("lj_user_login"));
                
                // Verify the user is who they claim to be.
                if(user->check_cookie(request->cookie("lj_user_cookie"))) {
                    
                    // User is authenticated, setup the context.
                    request->context_object("_user", user, true);
                    request->attribute("authenticated", "true");
                } else {
                    response->cookie("lj_user_login", "", 0, true);
                    response->cookie("lj_user_cookie", "", 0, true);
                    delete user;
                }
            } catch(const std::string &ex) {
                response->cookie("lj_user_login", "", 0, true);
                response->cookie("lj_user_cookie", "", 0, true);
            } catch(tokyo::Exception &ex) {
                response->cookie("lj_user_login", "", 0, true);
                response->cookie("lj_user_cookie", "", 0, true);
            }
        }
        
        // Now see if the user is logging in
        if(request->has_param("openid_url")) {
            std::string identity = request->param("openid_url");
            LogJamminConsumer relay_provider(identity);
            //openid_1_1::DumbRelayProvider relay_provider(identity);
            try {
                User user(relay_provider.identifier());
                
                // Add the login count to the return URL, won't allow a login
                // with out it.
                std::ostringstream target;
                target << request->original_request_file() << "?login_count=" << user.login_count();
                response->redirect(relay_provider.checkid_setup(target.str(),
                                                                request->original_request_script()));
            } catch(const std::string &ex) {
                std::cerr << "Exception loging in " << ex << std::endl;
                response->cookie("lj_user_login", "", 0, true);
                response->cookie("lj_user_cookie", "", 0, true);
            } catch(tokyo::Exception &ex) {
                std::cerr << "Exception loging in " << ex.msg << std::endl;
                response->cookie("lj_user_login", "", 0, true);
                response->cookie("lj_user_cookie", "", 0, true);
            }
        } else if(request->param("openid.mode").compare("id_res") == 0 &&
                  request->has_param("login_count")) {
            try {
                // Get the OpenId provider.
                std::string identity = request->param("openid.identity");
                LogJamminConsumer relay_provider(identity);
                //openid_1_1::DumbRelayProvider relay_provider(identity);
                
                // Get the user to check the login count.
                User *user = new User(relay_provider.identifier());
                std::ostringstream data;
                data << user->login_count();
                
                // Verify the user authentication.
                if(request->param("login_count").compare(data.str()) == 0 &&
                   relay_provider.check_authentication(request->params())) {
                    
                    // User is authenticated, setup the context.
                    request->context_object("_user", user, true);
                    request->attribute("authenticated", "true");
                    
                    // Create the random string for the cookie.
                    std::string ingredients;
                    for(int h = 0; h < 50; ++h) {
                        char x = (char)((rand() & 89) + 33);
                        if(x != '=' && x != ';' && x != '^')
                            ingredients.push_back(x);
                    }
                    
                    // Store the updated cookie in the DB.
                    user->cookie(ingredients);
                    user->incr_login_count();
                    user->save();
                    
                    // Send the cookies in the response.
                    response->cookie("lj_user_login",
                                     request->param("openid.identity"), 
                                     36000,
                                     true);
                    response->cookie("lj_user_cookie",
                                     ingredients,
                                     36000,
                                     true);
                    
                    // Redirect to where they were trying to go.
                    response->redirect(request->original_request_file());
                } else {
                    std::cerr << "Exception loging in. Check auth returned false." << std::endl;
                    delete user;
                    response->cookie("lj_user_login", "", 0, true);
                    response->cookie("lj_user_cookie", "", 0, true);
                }
            } catch(const std::string &ex) {
                std::cerr << "Validation failed " << ex << "." << std::endl;
                response->cookie("lj_user_login", "", 0, true);
                response->cookie("lj_user_cookie", "", 0, true);
            } catch(tokyo::Exception &ex) {
                std::cerr << "Exception loging in " << ex.msg << std::endl;
                response->cookie("lj_user_login", "", 0, true);
                response->cookie("lj_user_cookie", "", 0, true);
            }
        }
    } else {
        response->cookie("lj_user_login", "", 0, true);
    }
}

bool HttpHeadersFilter::is_requested(CGI::Request *request, CGI::Response *response) {
    return true;
}

void HttpHeadersFilter::execute(CGI::Request *request, CGI::Response *response) {
    response->content_type("text/html; charset=UTF-8");
}

bool MessageExpanderFilter::is_requested(CGI::Request *request, CGI::Response *response) {
    return request->has_param("_msg");
}

void MessageExpanderFilter::execute(CGI::Request *request, CGI::Response *response) {
    std::string msg_code = request->param("_msg");
    
    if(msg_code.compare("SAVE_SUCCESS") == 0) {
        request->attribute("_msg", "Save Successful.");
        request->attribute("_msg_class", "success");
    } else if(msg_code.compare("PURGE_SUCCESS") == 0) {
        request->attribute("_msg", "Purge Successful.");
        request->attribute("_msg_class", "success");
    }
}

bool TemplateTopFilter::is_requested(CGI::Request *request, CGI::Response *response) {
    if(request->header("HTTP_X_REQUESTED_WITH").compare("XMLHttpRequest") == 0)
        return false;
    return true;
}

void TemplateTopFilter::execute(CGI::Request *request, CGI::Response *response) {
    if(request->has_attribute("authenticated")) {
        response->execute("header-auth.html", request);
    } else {
        response->execute("header-unauth.html", request);
    }
}

bool TemplateBottomFilter::is_requested(CGI::Request *request, CGI::Response *response) {
    if(request->header("HTTP_X_REQUESTED_WITH").compare("XMLHttpRequest") == 0)
        return false;
    return true;
}

void TemplateBottomFilter::execute(CGI::Request *request, CGI::Response *response) {
    if(request->has_attribute("authenticated")) {
        response->execute("footer-auth.html", request);
    } else {
        response->execute("footer-unauth.html", request);
    }
}

bool NotFoundController::is_requested(CGI::Request *request, CGI::Response *response) {
    return !(request->has_attribute("handled"));
}

void NotFoundController::execute(CGI::Request *request, CGI::Response *response) {
    response->status(404);
    response->execute("error-404.html", request);
    request->attribute("handled");
}
