#pragma once
/*
 \file Logger.h
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

#include <ostream>
#include <string>

namespace lj
{
    //! Logger base class.
    /*!
     \par
     This logger does nothing when disabled. When enabled, it returns a real
     logger that actually outputs data.
     \author Jason Watson
     \version 1.0
     \date April 15, 2010
     */
    class Log {
    public:
    
        //! Different logging levels.
        enum Event_level
        {
            level_emergency, //!< emergency event level.
            level_alert,     //!< alert event level.
            level_critical,  //!< critical event level.
            level_error,     //!< error event level.
            level_warning,   //!< warning event level.
            level_notice,    //!< notice event level.
            level_info,      //!< info event level.
            level_debug      //!< debug event level.
        };
        
        //! End the log message and write it out.
        /*!
         \par
         This is used to end building a log message.  When the underlying logger
         receives this object, it should flush the log message and delete
         itself.
         */
        struct End
        {
        };
        
        //! convert an event level into a string.
        /*!
         \param level The event level to convert.
         \return The string describing the event level.
         */
        static std::string level_text(const Event_level level);
        
        //! Create a new default logger.
        /*!
         \param lvl The event level associated with this logger.
         \param s The stream to use for output messages.
         */
        Log(Event_level lvl, std::ostream *s) : level_(lvl), stream_(s), enabled_(true)
        {
        }
        
        //! Logger destructor
        virtual ~Log()
        {
        }
        
        //! Disable logging.
        /*!
         \return The current Log.
         */
        Log &disable()
        {
            enabled_  = false;
            return *this;
        }
        
        //! Enable logging.
        /*!
         \return The current Log.
         */
        Log &enable()
        {
            enabled_ = true;
            return *this;
        }
        
        //! Log a message to the output stream.
        /*!
         \par
         Log the given message to the output stream.  If additional parameters
         are provided, they are substituted into the strin before writing
         it to the output stream.
         \code
         Log::debug("X = %d and Y = %d for %s", x, y, "foo");
         \endcode
         \param fmt The format of the log message.
         \param ... The arguments to use for formatting.
         */
        void operator()(const std::string &fmt, ...);
        
        //! Build a message for the output stream.
        /*!
         \par
         A Log object is loaded with the provided format and returned for
         passing arguments to. When all arguments have been passed to the
         Log object, the caller should pass \c Log::end as an argument to
         flush the message to the output stream.
         \par
         The logger deletes itself when it is passed \c Log::end.
         \code
         Log::debug.log("X = %d and Y = %d for %s") << x << y << "foo" << Log::end;
         Log* log = &Log::debug.log("%s logging in %d times");
         (*log) << "fooUser";
         (*log) << 24;
         (*log) << Log::end;
         \endcode
         \param fmt The format of the log message.
         \return The current Log.
         */
        Log &log(const std::string &fmt);
        
        //! Log a value.
        virtual Log &operator<<(const std::string &msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(const char *msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(long long msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(unsigned long long msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(long msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(unsigned long msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(int msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(unsigned int msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(short msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(unsigned short msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(char msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(unsigned char msg) { return *this; };
        //! Log a value.
        virtual Log &operator<<(bool msg) { return *this; };
        //! Close the logger.
        virtual void operator<<(const End &msg) { };
        
        
        static Log emergency; //!< Emergency event logger.
        static Log alert;     //!< Alert event logger.
        static Log critical;  //!< Critical event logger.
        static Log error;     //!< Error event logger.
        static Log warning;   //!< Warning event logger.
        static Log notice;    //!< Notice event logger.
        static Log info;      //!< Info event logger.
        static Log debug;     //!< Debug event logger.
        static End end;       //!< End object.
    protected:
        Event_level level_;    //!< event level associated with the logger.
        std::ostream* stream_; //!< stream to output the log messages to.
        bool enabled_;         //!< enabled flag.
    };
}; // namespace lj