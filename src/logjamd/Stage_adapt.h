#pragma once
/*!
 \file Stage_adapt.h
 \brief Logjam server abstract class for adapter stages.
 \author Jason Watson

 Copyright (c) 2010, Jason Watson
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

#include "lj/Streambuf_pipe.h"
#include "logjamd/Connection.h"
#include "logjamd/Stage.h"

namespace logjamd
{
    //! Provides an adapter between different protocols.
    /*!
     \par
     The native format for talking to a logjamd server is using serialized
     bson objects. In order to provide telnet and http access to the server,
     a small adapter must be created to do the translation.
     \par
     This class provides the basic infrastructure for adapters to json
     and HTTP.
     \since 0.2
     \sa logjamd::Stage_http_adapt
     \sa logjamd::Stage_json_adapt
     */
    class Stage_adapt : public Stage
    {
    public:
        Stage_adapt(logjamd::Connection* connection);
        virtual ~Stage_adapt();
        virtual Stage* logic() = 0;
        virtual std::string name() = 0;

        //! The language to use for scripting.
        /*!
         \return Reference to the language used by this adapter.
         */
        virtual const std::string& language() const
        {
            return language_;
        }

        //! The fake connection used for the real stage.
        /*!
         \return Reference to the fake connection object.
         */
        virtual Connection& faux_connection()
        {
            return faux_connection_;
        }

        //! Get a new Auth stage attached to the pipe and connection.
        /*!
         \par
         It is up to the adapter stage to manage the stage lifecycle.
         \par
         The caller takes ownership of the memory associated with this
         pointer.
         \return Returns a newly allocated pointer to an Auth Stage.
         \sa logjamd::Stage_http_adapt
         \sa logjamd::Stage_json_adapt
         */
        virtual std::unique_ptr<Stage> new_auth_stage();
    protected:
        //! The pipe used to back the faux connection.
        /*!
         \return Reference to the pipe object of the faux connection.
         */
        virtual lj::Streambuf_pipe& pipe()
        {
            return pipe_;
        }

        //! Set the language to use for the real executor stage.
        /*!
         \param language The language to use.
         */
        virtual void set_language(const std::string& language);
    private:
        lj::Streambuf_pipe pipe_;
        Connection_xlator faux_connection_;
        std::string language_;
    };
};

