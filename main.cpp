/*
 \file main.cpp
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


#include <iostream>
#include <list>
#include "Model.h"
#include "Request.h"
#include "Response.h"
#include "Controller.h"
#include "ProjectController.h"
#include "BacklogController.h"
#include "UserController.h"
#include "Seed.h"
#include "RssController.h"
#include "RoleController.h"

int main (int argc, char * const argv[]) {
        
    // Create the request/response wrappers.
    CGI::Request *request = new CGI::Request();
    CGI::Response *response = new CGI::Response();
    
    request->context_object("request", request, false);
    request->context_object("response", response, false);
    
    // Create an array of the controllers in the order to evaluate.
    logjammin::controller::Controller *controllers[] = {
        new logjammin::controller::AuthenticateFilter(),
        new logjammin::controller::HttpHeadersFilter(),
        new logjammin::controller::MessageExpanderFilter(),
        new logjammin::controller::TemplateTopFilter(),
        new logjammin::controller::Seed(),
        new logjammin::controller::StaticAssetController(),
        new logjammin::controller::BacklogEditController(),
        new logjammin::controller::BacklogSearchController(),
        new logjammin::controller::BacklogListController(),
        new logjammin::controller::ProjectEditController(),
        new logjammin::controller::ProjectSearchController(),
        new logjammin::controller::ProjectPurgeController(),
        new logjammin::controller::ProjectListController(),
        new logjammin::controller::UserEditController(),
        new logjammin::controller::UserSearchController(),
        new logjammin::controller::UserPurgeController(),
        new logjammin::controller::UserListController(),
        new logjammin::controller::RoleEditController(),
        new logjammin::controller::RolePurgeController(),
        new logjammin::controller::RoleListController(),
        new logjammin::controller::CommitFeedController(),
        new logjammin::controller::NotFoundController(),
        new logjammin::controller::TemplateBottomFilter(),
        0
    };

    try {
        for(logjammin::controller::Controller **iter = controllers; *iter; ++iter) {
            if((*iter)->is_requested(request, response))
                (*iter)->execute(request, response);
            if(response->is_closed())
                break;
        }
    } catch(std::string &ex) {
        std::cerr << ex << std::endl;
        response->status(500);
    } catch(tokyo::Exception &ex) {
        std::cerr << ex.msg << std::endl;
        response->status(500);
    }
    
    response->close();
    delete response;
    return 0;
}