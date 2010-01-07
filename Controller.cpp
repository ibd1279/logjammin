/*
 \file Controller.cpp
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

#include <stdlib.h>
#include <time.h>
#include "Controller.h"
#include "User.h"
#include "OpenID.h"
#include "OpenIDConsumer.h"
#include <iostream>

namespace logjammin {
    namespace controller {
        
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
                        if(user->check_cookie(request->cookie("lj_user_cookie")) || request->has_attribute("gdb_mode")) {
                            
                            // User is authenticated, setup the context.
                            request->context_object("_user", user, true);
                            request->attribute("authenticated", "true");
                        } else {
                            response->cookie("lj_user_login", "", request->script_name(), 0, true);
                            response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                            delete user;
                        }
                    } catch(const std::string &ex) {
                        response->cookie("lj_user_login", "", request->script_name(), 0, true);
                        response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                    } catch(tokyo::Exception &ex) {
                        response->cookie("lj_user_login", "", request->script_name(), 0, true);
                        response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                    }
                }
                
                // Now see if the user is logging in
                if(request->has_param("openid_url")) {
                    std::string identity = request->param("openid_url");
                    OpenIDConsumer relay_provider(identity);
                    try {
                        User user(relay_provider.identifier());
                        
                        // Add the login count to the return URL, won't allow a login
                        // with out it.
                        std::ostringstream target;
                        target << request->original_request_script() << "/?login_count=" << user.login_count();
                        target << "&_qs=" << CGI::Response::percent_encode(request->param("_qs"));
                        target << "&_pi=" << CGI::Response::percent_encode(request->param("_pi"));
                        
                        response->redirect(relay_provider.checkid_setup(target.str(),
                                                                        request->original_request_script()));
                    } catch(const std::string &ex) {
                        std::cerr << "Exception loging in " << ex << std::endl;
                        response->cookie("lj_user_login", "", request->script_name(), 0, true);
                        response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                    } catch(tokyo::Exception &ex) {
                        std::cerr << "Exception loging in " << ex.msg << std::endl;
                        response->cookie("lj_user_login", "", request->script_name(), 0, true);
                        response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                    }
                } else if(request->param("openid.mode").compare("id_res") == 0 &&
                          request->has_param("login_count")) {
                    try {
                        // Get the OpenId provider.
                        std::string identity = request->param("openid.identity");
                        OpenIDConsumer relay_provider(identity);
                        
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
                            std::cerr << "post login 5" << std::endl;
                            user->save();
                            std::cerr << "post login 6" << std::endl;
                            
                            // Send the cookies in the response.
                            response->cookie("lj_user_login",
                                             request->param("openid.identity"), 
                                             request->script_name(),
                                             36000,
                                             true);
                            response->cookie("lj_user_cookie",
                                             ingredients,
                                             request->script_name(),
                                             36000,
                                             true);
                            
                            // Redirect to where they were trying to go.
                            std::ostringstream target;
                            target << request->original_request_script();
                            target << request->param("_pi");
                            target << "?" << request->param("_qs");
                            response->redirect(target.str());
                        } else {
                            std::cerr << "Exception loging in. Check auth returned false." << std::endl;
                            delete user;
                            response->cookie("lj_user_login", "", request->script_name(), 0, true);
                            response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                        }
                    } catch(const std::string &ex) {
                        std::cerr << "Validation failed " << ex << "." << std::endl;
                        response->cookie("lj_user_login", "", request->script_name(), 0, true);
                        response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                    } catch(tokyo::Exception &ex) {
                        std::cerr << "Exception loging in " << ex.msg << std::endl;
                        response->cookie("lj_user_login", "", request->script_name(), 0, true);
                        response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
                    }
                }
            } else {
                response->cookie("lj_user_login", "", request->script_name(), 0, true);
                response->cookie("lj_user_cookie", "", request->script_name(), 0, true);
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
            request->attribute("handled", "true");
        }
        
        bool StaticAssetController::is_requested(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            if(args.size() < 2)
                return false;
            return (args.front().compare("static") == 0);
        }
        
        void StaticAssetController::execute(CGI::Request *request, CGI::Response *response) {
            response->header("Cache-Control", "max-age=3600, public");
            response->stream(request->path_info(), request);
            request->attribute("handled", "true");
        }
    }; // namespace controller
}; // namespace logjammin