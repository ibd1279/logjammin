#pragma once
#include "Controller.h"

class BacklogListController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class BacklogEditController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class BacklogPurgeController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class BacklogSearchController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};
