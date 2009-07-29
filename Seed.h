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
        
        Role role;
        role.name("Administrator");
        role.save();
        
        Role role2;
        role2.name("User");
        role2.save();
        
        User user;
        user.name("Jason Watson");
        user.logins().push_back("http://openid.aol.com/jasonwatson06");
        user.email("jwatson@slashopt.net");
        user.role(role);
        user.save();
        
        User user2;
        user2.name("Hyoo Lim");
        user2.logins().push_back("http://openid.aol.com/hyoolim08");
        user2.email("hyoolim@gmail.com");
        user2.role(role);
        user2.save();
        
        User user3;
        user3.name("Jeremy Collins");
        user3.logins().push_back("http://openid.aol.com/jeremycollins11");
        user3.email("Jeremy.Collins@corp.aol.com");
        user3.role(role);
        user3.save();

        response->redirect(request->original_request_script());
    }
};
