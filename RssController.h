#pragma once
#include "Controller.h"

class CommitFeedController : public Controller {
public:
    virtual bool is_requested(CGI::Request *request, CGI::Response *response);
    virtual void execute(CGI::Request *request, CGI::Response *response);
};