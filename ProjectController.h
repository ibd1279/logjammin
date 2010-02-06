#pragma once
/*
 \file ProjectController.h
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

namespace logjammin {
    namespace controller {
        //! Controller for listing project information.
        /*!
         \author Jason Watson
         \version 1.0
         \date January 15, 2010.
         */
        class ProjectListController : public Controller {
        public:
            virtual bool is_requested(CGI::Request *request, CGI::Response *response);
            virtual void execute(CGI::Request *request, CGI::Response *response);
        };
        
        //! Controller for listing project information.
        /*!
         \author Jason Watson
         \version 1.0
         \date August 1, 2009.
         */
        class ProjectEditController : public Controller {
        public:
            virtual bool is_requested(CGI::Request *request, CGI::Response *response);
            virtual void execute(CGI::Request *request, CGI::Response *response);
        };
        
        //! Controller for deleting projects.
        /*!
         \par XXX Clean up?
         Could this be moved into the edit controller as another code path?
         or is this seperation a better long term division.  It is all in the
         same file after all, and the edit control is already relatively
         complex.
         \author Jason Watson
         \version 1.0
         \date August 10, 2009.
         */
        class ProjectPurgeController : public Controller {
        public:
            virtual bool is_requested(CGI::Request *request, CGI::Response *response);
            virtual void execute(CGI::Request *request, CGI::Response *response);
        };
    }; // namespace logjammin::controller
}; // namespace controller
