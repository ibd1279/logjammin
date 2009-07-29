#pragma once
#include "Controller.h"

class ProjectListController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class ProjectEditController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class ProjectPurgeController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class ProjectSearchController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

