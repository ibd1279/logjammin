#pragma once
/*!
 \file logjam/Tls_globals.h
 \brief Logjam TLS globals header file.
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

#include "lj/Exception.h"

namespace logjam
{

    //! TLS Exception class.
    /*!
     \par
     Wrapper for Tls exceptions. Resolves the TLS error code into a string
     as part of the message.
     \author Jason Watson
     \since 0.2
     \date September 26, 2012
     */
    class Tls_exception : public lj::Exception
    {
    public:
        //! Standard constructor.
        /*!
         \par
         Tls Exception will convert the code into a string. \c msg should be
         something to help explain where the exception was caused, not what
         the TLS error was.
         \param msg The message.
         \param code The TLS error code.
         */
        Tls_exception(const std::string& msg, int code) : lj::Exception("Tls", msg),
                code_(code)
        {
        }

        //! Copy constructor.
        /*!
         \param o The other object.
         */
        Tls_exception(const Tls_exception& o) : lj::Exception(o),
                code_(o.code_)
        {
        }

        //! Move constructor
        /*!
         \param o The other object.
         */
        Tls_exception(Tls_exception&& o) : lj::Exception(o),
                code_(o.code_)
        {
        }

        //! Destructor
        virtual ~Tls_exception()
        {
        }

        //! Copy Assignment operator
        /*!
         \param o The other object.
         */
        Tls_exception& operator=(const Tls_exception& o)
        {
            if (&o != this)
            {
                lj::Exception::operator=(o);
                code_ = o.code_;
            }
            return *this;
        }

        //! Move assignment operator
        /*!
         \param o The other object
         */
        Tls_exception& operator=(Tls_exception&& o)
        {
            if (&o != this)
            {
                lj::Exception::operator=(o);
                code_ = o.code_;
            }
            return *this;
        }

        //! Get the exception string.
        /*!
         \par
         Returns a std::string that represents this exception. This includes
         converting any error codes into strings and appending that string
         to the exception message.
         \return A string representing this TLS exception.
         */
        virtual std::string str() const override;

        //! Get the TLS error code.
        /*!
         \return The raw TLS error code.
         */
        virtual int code() const
        {
            return code_;
        }
    private:
        int code_;
    };

    //! TLS global initialization structure.
    /*!
     Every application using TLS needs to create one of these objects. The
     object is not used by the rest of the TLS api, but it does setup some
     datastructures necessary for the rest of library.
     */
    class Tls_globals
    {
    public:
        Tls_globals();
        Tls_globals(const Tls_globals&) = delete;
        ~Tls_globals();
        Tls_globals& operator=(const Tls_globals&) = delete;
    };
}; // namespace logjam
