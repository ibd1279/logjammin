#pragma once
/*!
 \file lj/Exception.h
 \brief LJ Exception header and implementation.
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

#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace lj
{
    //! Exception base class.
    /*!
     \par
     Bubble up Exception type for exceptions in logjam.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    class Exception : public std::exception
    {
    public:
        //! Create a new exception object.
        /*!
         \param lbl The type of exception.
         \param msg Exception message.
         */
        Exception(const std::string& lbl,
                const std::string& msg) : what_cache_(), msg_(msg), label_(lbl)
        {
        }

        //! Destructor
        virtual ~Exception() throw()
        {
        }

        //! Copy constructor.
        /*!
         \param o Other.
         */
        Exception(const Exception& o) : what_cache_(), msg_(o.msg_), label_(o.label_)
        {
        }

        //! Move constructor
        /*!
         \param o Other object.
         */
        Exception(Exception&& o) : what_cache_(std::move(o.what_cache_)),
                msg_(std::move(o.msg_)),
                label_(std::move(o.label_))
        {
        }

        //! Copy Assignment Operator
        /*!
         \param o Other.
         */
        Exception& operator=(const Exception& o)
        {
            if (&o != this)
            {
                msg_ = o.msg_;
                label_ = o.label_;
                what_cache_.reset(nullptr);
            }
            return *this;
        }

        //! Move Assignment Operator
        /*!
         \param o The other object.
         */
        Exception& operator=(Exception&& o)
        {
            if (&o != this)
            {
                msg_ = std::move(o.msg_);
                label_ = std::move(o.label_);
                what_cache_ = std::move(o.what_cache_);
            }
            return *this;
        }

        //! Convert the exception to a string.
        /*!
         \par
         This method should be overridden to provide a more detailed error
         message.
         \return String for the exception.
         */
        virtual std::string str() const
        {
            return std::string(label_).append(" Exception: ").append(msg_);
        }

        //! Convert the exception to a string.
        /*!
         \return String for the exception.
         \sa lj::Exception::str()
         */
        virtual operator std::string() const
        {
            return str();
        }

        //! replace the default "what" method to call str()
        /*!
         \par Memory
         The pointer obtained from \c what() is only valid until the next call
         to \c what(), and while the \c Exception object is still valid.
         \return String for the exception.
         */
        virtual const char* what() const throw()
        {
            // See the what_cache_ field for details as to why this is mutable.
            what_cache_.reset(new std::string(str()));
            return what_cache_->c_str();
        }
    protected:

        //! What result cache.
        /*!
         This field is to support the \c what() method. It is used to hang onto
         a string*, so that the char* returned by \c what() is not released
         immediately.
         */
        mutable std::unique_ptr<std::string> what_cache_;

        //! Exception message.
        std::string msg_;

        //! Exception label.
        std::string label_;
    };
}; // namespace lj
#define LJ__Exception(msg) lj::Exception(__FILE__, std::string(__FUNCTION__).append(" - ").append(msg))
