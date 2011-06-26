#pragma once
/*!
 \file lj/Log.h
 \brief LJ Logger header.
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

#include "lj/Uuid.h"

#include <cstdint>
#include <exception>
#include <ostream>
#include <sstream>
#include <string>

namespace lj
{
    //! Log base class.
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
        /*!
         \par
         These levels are based on syslog levels.
         */
        enum class Level
        {
            k_emergency, //!< emergency event level.
            k_alert,     //!< alert event level.
            k_critical,  //!< critical event level.
            k_error,     //!< error event level.
            k_warning,   //!< warning event level.
            k_notice,    //!< notice event level.
            k_info,      //!< info event level.
            k_debug      //!< debug event level.
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
        
        //! Convert an event level into a string.
        /*!
         \param level The event level to convert.
         \return The string describing the event level.
         */
        static const std::string& level_text(const Level& level);

        //! Convert an event level into a Log object.
        /*!
         \param level The event level to convert.
         \return The Log object for that event level.
         */
        static Log& for_level(const Level& level);
        
        //! Create a new log.
        /*!
         \par
         The provided std::ostream is not managed by the Log object. The application
         will need to release the pointer when shutting down.
         \param lvl The event level associated with this logger.
         \param s The stream to use for output messages.
         */
        Log(Level lvl, std::ostream *s) : level_(lvl), stream_(s), enabled_(true)
        {
        }
        
        //! Log destructor
        virtual ~Log()
        {
        }
        
        //! Disable logging.
        /*!
         \return The current Log.
         */
        Log& disable()
        {
            enabled_  = false;
            return *this;
        }
        
        //! Enable logging.
        /*!
         \return The current Log.
         */
        Log& enable()
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
        template <class ...Args>
        void log(const std::string& fmt, const Args& ...args)
        {
            Log& logger = (*this)(fmt);
            logger.sub_log(args...);
        }
        
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
        Log& operator()(const std::string& fmt);
        
        //! Write a string to the output stream.
        /*!
         \param msg The message to write.
         \return The current Log.
         */
        virtual Log& write_string(const std::string& msg)
        {
            return *this;
        }
        
        //! Write a signed integer to the output stream.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        virtual Log& write_signed_int(const int64_t msg)
        {
            return *this;
        }
        
        //! Write an unsigned integer to the output stream.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        virtual Log& write_unsigned_int(const uint64_t msg)
        {
            return *this;
        }
        
        //! Write a bool to the output stream.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        virtual Log& write_bool(const bool msg)
        {
            return *this;
        }
        
        //! Write a pointer address to the output stream.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        virtual Log& write_pointer(const void* msg)
        {
            return *this;
        }
        
        //! End the current log message.
        /*!
         \return The current Log.
         */
        virtual void write_end()
        {
        }
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const std::string& msg) { return write_string(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const char* msg) { return write_string(msg == 0 ? std::string("NULL") : std::string(msg)); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const int64_t msg) { return write_signed_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const uint64_t msg) { return write_unsigned_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const int32_t msg) { return write_signed_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const uint32_t msg) { return write_unsigned_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const int16_t msg) { return write_signed_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const uint16_t msg) { return write_unsigned_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const int8_t msg) { return write_signed_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const uint8_t msg) { return write_unsigned_int(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const bool msg) { return write_bool(msg); };
        
        //! Log a value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const void* msg) { return write_pointer(msg); };
        
        //! Log a unique id value.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline Log& operator<<(const Uuid& msg) { return write_string(msg.str()); };
        
        //! Log an exception.
        /*!
         \param ex The exception to log
         \return The current Log.
         */
        inline Log& operator<<(const std::exception& ex) { return write_string(ex.what()); };

        //! Close the logger.
        /*!
         \param msg The message to write to the output.
         \return The current Log.
         */
        inline void operator<<(const End &msg) { write_end(); };
        
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
        Level level_;    //!< event level associated with the logger.
        std::ostream* stream_; //!< stream to output the log messages to.
        bool enabled_;         //!< enabled flag.
    private:
        void sub_log()
        {
            (*this) << end;
        }

        template <class A0, class ...Args>
        void sub_log(const A0& a0, const Args& ...args)
        {
            (*this) << a0;
            sub_log(args...);
        }

    };

    //! Wrapper to execute a function and log exceptions.
    /*!
     \author Jason Watson <jwatson@slashopt.net>
     \date June 25, 2011
     */
    template <class F>
    class Log_exception
    {
    public:
        Log_exception(lj::Log& log, F func) : log_(log), func_(func)
        {
        }

        void operator()()
        {
            try
            {
                func_();
            }
            catch (const std::exception& ex)
            {
                log_("Unhandled exception: %s") << ex << Log::end;
            }
            catch (std::exception* ex)
            {
                log_("Unhandled exception: %s") << *ex << Log::end;
                delete ex;
            }
            catch (...)
            {
                log_("Unhandled exception of unknown type.") << Log::end;
            }
        }
    private:
        Log& log_;
        F func_;
    };
}; // namespace lj
