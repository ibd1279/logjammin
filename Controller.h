#pragma once
#include "Request.h"
#include "Response.h"

class Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response) = 0;
    virtual void execute(CGI::Request *request, CGI::Response *response) = 0;
};

class AuthenticateFilter : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class HttpHeadersFilter : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class MessageExpanderFilter : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class TemplateTopFilter : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class TemplateBottomFilter : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};

class NotFoundController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};
