#pragma once
#include "Controller.h"

class UserListController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class UserEditController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class UserPurgeController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};
