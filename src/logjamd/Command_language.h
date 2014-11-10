#pragma once
/*!
 \file logjamd/Command_language.h
 \brief Logjam server networking header.
 \author Jason Watson

 Copyright (c) 2014, Jason Watson
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

#include "logjam/Pool.h"
#include "lj/Bson.h"
#include <string>

namespace logjamd
{
    //! Abstract base class for implementing command languages
    /*!
     \par
     Command_language objects are expected to be stateful. A new object
     is created for every invocation.
     */
    class Command_language
    {
    public:
        Command_language() = default;
        Command_language(const Command_language& o) = default;
        Command_language(Command_language&& o) = default;
        Command_language& operator=(const Command_language& rhs) = default;
        Command_language& operator=(Command_language&& rhs) = default;
        virtual ~Command_language() = default;

        //! Perform the requested command.
        /*!
         \par
         Executes the command part of the request. Any manipulation of the
         response would be included in this method.
         \param[out] response The response to the client.
         \return True if the connection should stay open. False to close.
         */
        virtual bool perform(logjam::pool::Swimmer& swmr,
                lj::bson::Node& request,
                lj::bson::Node& response) const = 0;

        //! Name of the command language. Used for logging.
        /*!
         \return The friendly name of this command language.
         */
        virtual std::string name() const = 0;
    };
};
