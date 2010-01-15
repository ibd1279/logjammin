#pragma once
/*
 \file Controller.h
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

#include "Request.h"
#include "Response.h"

namespace logjammin {
    namespace controller {
        
        //! Controller base class.
        /*!
         \author Jason Watson
         \version 1.0
         \date July 12, 2009.
         */
        class Controller {
        public:
            //! Destructor
            virtual ~Controller() { };
            
            //! Check if this controller was requested.
            /*!
             \par
             Controllers and filters are expected to test the request and
             return true if the controller should be executed.
             \param request The request wrapper.
             \param response The response wrapper.
             \return True if the controller should execute. false otherwise.
             \sa execute()
             */
            virtual bool is_requested(CGI::Request *request, CGI::Response *response) = 0;
            
            //! Execute the logic for this controller.
            /*!
             \par
             Execute the logic for this controller. Typically this modifies the
             request object, and outputs a response through the response object.
             \param request The request wrapper.
             \param response The response wrapper.
             \sa is_requested()
             */
            virtual void execute(CGI::Request *request, CGI::Response *response) = 0;
        };
        
        //! Authentication Filter
        /*!
         \par Authenticates a user and grants access or not.  Currently uses OpenID.
         \author Jason Watson
         \version 1.0
         \date August 1, 2009.
         */
        class AuthenticateFilter : public Controller {
        public:
            virtual bool is_requested(CGI::Request *request, CGI::Response *response);
            virtual void execute(CGI::Request *request, CGI::Response *response);
        };
        
        //! Impersonation Filter
        /*!
         \par A user with specific access is allowed to impersonate other users.
         This filter enables that functionality.
         \author Jason Watson
         \version 1.0
         \date January 15, 2009.
         */
        class ImpersonationFilter : public Controller { 
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
        class StaticAssetController : public Controller {
        public:
            virtual bool is_requested(CGI::Request *request, CGI::Response *response);
            virtual void execute(CGI::Request *request, CGI::Response *response);
        };
    }; // namespace controller
}; // namespace logjammin