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

int main (int argc, char * const argv[]) {
        
    // Create the request/response wrappers.
    CGI::Request *request = new CGI::Request();
    CGI::Response *response = new CGI::Response();
    
    request->context_object("request", request, false);
    request->context_object("response", response, false);
    
    // Create an array of the controllers in the order to evaluate.
    Controller *controllers[] = {
        new AuthenticateFilter(),
        new HttpHeadersFilter(),
        new MessageExpanderFilter(),
        new TemplateTopFilter(),
        new Seed(),
        new BacklogEditController(),
        new BacklogSearchController(),
        new BacklogListController(),
        new ProjectEditController(),
        new ProjectSearchController(),
        new ProjectPurgeController(),
        new ProjectListController(),
        new UserEditController(),
        new UserSearchController(),
        new UserPurgeController(),
        new UserListController(),
        new NotFoundController(),
        new TemplateBottomFilter(),
        0
    };

    try {
        for(Controller **iter = controllers; *iter; ++iter) {
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