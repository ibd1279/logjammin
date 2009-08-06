#pragma once
/*
 \file Seed.h
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

#include "Controller.h"
#include "Project.h"
#include "Role.h"
#include "User.h"

class Seed : public Controller {
private:
    std::list<std::string> args;
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response) {
        std::list<std::string> args = request->split_path_info();
        if(args.size() < 1) return false;
        return (args.back().compare("__seed") == 0);
    }
    virtual void execute(CGI::Request *request, CGI::Response *response) {
        Project logjammin;
        logjammin.name("Logjammin");
        logjammin.versions().push_back("1.0.0");
        logjammin.categories().push_back("New Feature");
        logjammin.categories().push_back("Enhancement");
        logjammin.categories().push_back("Bug");
        logjammin.save();
        
        Role admin_role;
        admin_role.name("Administrator");
        admin_role.allowed().push_back("admin:user:read");
        admin_role.allowed().push_back("admin:user:write");
        admin_role.allowed().push_back("admin:role:read");
        admin_role.allowed().push_back("admin:role:write");
        admin_role.save();
        
        Role role2;
        role2.name("User");
        role2.save();
        
        User user;
        user.name("Jason Watson");
        user.logins().push_back("http://openid.aol.com/jasonwatson06");
        user.logins().push_back("http://openid.aol.com/ibd1279");
        user.email("jwatson@slashopt.net");
        user.role(admin_role);
        user.save();
        
        User user2;
        user2.name("Hyoo Lim");
        user2.logins().push_back("http://openid.aol.com/hyoolim08");
        user2.email("hyoolim@gmail.com");
        user2.role(admin_role);
        user2.save();
        
        User user3;
        user3.name("Jeremy Collins");
        user3.logins().push_back("http://openid.aol.com/jeremycollins11");
        user3.email("Jeremy.Collins@corp.aol.com");
        user3.role(admin_role);
        user3.save();

        response->redirect(request->original_request_script());
    }
};
