#pragma once
/*!
 \file lj/Log.h
 \brief LJ Log header.
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
#include <list>
#include <sstream>
#include <string>

namespace lj
{
    //! Base class for Logging Levels.
    /*!
     \version 1.0
     \date November 16, 2011
     */
    class LogLevel
    {
    private:
        const std::string name_;
        const int level_;
    public:
        //! Create a new logging level.
        /*!
         \param n The logging level name.
         \param l The logging level number.
         */
        LogLevel(const char* n,
                const int l) :
                name_(n),
                level_(l)
        {
        }

        //! standard destructor
        virtual ~LogLevel()
        {
        }

        //! Get the logging level name.
        /*!
         \return The logging level name.
         */
        const std::string& name() const
        {
            return name_;
        }

        //! Get the logging level number.
        /*!
         \return The logging level number.
         */
        const int level() const
        {
            return level_;
        }
    };

    //! Emergency Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Emergency : public LogLevel
    {
        Emergency() : LogLevel("EMERGENCY", 0)
        {
        }
    };

    //! Alert Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Alert : public LogLevel
    {
        Alert() : LogLevel("ALERT", 1)
        {
        }
    };

    //! Critical Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Critical : public LogLevel
    {
        Critical() : LogLevel("CRITICAL", 2)
        {
        }
    };

    //! Error Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Error : public LogLevel
    {
        Error() : LogLevel("ERROR", 3)
        {
        }
    };

    //! Warning Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Warning : public LogLevel
    {
        Warning() : LogLevel("WARNING", 4)
        {
        }
    };

    //! Notice Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Notice : public LogLevel
    {
        Notice() : LogLevel("NOTICE", 5)
        {
        }
    };

    //! Info Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Info : public LogLevel
    {
        Info() : LogLevel("INFORMATION", 6)
        {
        }
    };

    //! Debug Logging Level
    /*!
     \version 1.0
     \date November 16, 2011
     */
    struct Debug : public LogLevel
    {
        Debug() : LogLevel("DEBUG", 7)
        {
        }
    };

    namespace log
    {
        //! type for closing a logger object.
        struct End
        {
        };

        extern End end;

        //! Logger Base class.
        /*!
         \par
         Provides a default implementation for logging, which is a no-op.
         */
        class Logger
        {
        public:
            //! Constructor used by the Log template.
            /*!
             \param lvl The logging level string.
             \param fmt The log formatting string.
             */
            Logger(const std::string lvl,
                    const std::string& fmt)
            {
            }

            //! Empty destructor.
            virtual ~Logger()
            {
            }
        protected:
            //! Write a string to the output stream.
            /*!
             \param msg The message to write.
             \return The current Log.
             */
            virtual Logger& write_string(const std::string& msg)
            {
                return *this;
            }
            
            //! Write a signed integer to the output stream.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_signed_int(const int64_t msg)
            {
                return *this;
            }
            
            //! Write an unsigned integer to the output stream.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_unsigned_int(const uint64_t msg)
            {
                return *this;
            }
            
            //! Write a bool to the output stream.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_bool(const bool msg)
            {
                return *this;
            }
            
            //! Write a pointer address to the output stream.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_pointer(const void* msg)
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
            
        public:
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const std::string& msg) { return write_string(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const char* msg) { return write_string(msg == 0 ? std::string("NULL") : std::string(msg)); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int64_t msg) { return write_signed_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint64_t msg) { return write_unsigned_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int32_t msg) { return write_signed_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint32_t msg) { return write_unsigned_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int16_t msg) { return write_signed_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint16_t msg) { return write_unsigned_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int8_t msg) { return write_signed_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint8_t msg) { return write_unsigned_int(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const bool msg) { return write_bool(msg); };
            
            //! Log a value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const void* msg) { return write_pointer(msg); };
            
            //! Log a unique id value.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const Uuid& msg) { return write_string(msg.str()); };
            
            //! Log an exception.
            /*!
             \param ex The exception to log
             \return The current Log.
             */
            inline Logger& operator<<(const std::exception& ex) { return write_string(ex.what()); };

            //! Close the logger.
            /*!
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline void operator<<(const End &msg) { write_end(); };

            //! Helper function to make logging a single function call.
            void end()
            {
                (*this) << lj::log::end;
            }

            //! Helper function to make logging a single function call.
            template <class A0, class ...Args>
            void end(const A0& a0, const Args& ...args)
            {
                (*this) << a0;
                end(args...);
            }
        };

        class Logger_stream : public Logger
        {
        public:
            //! Constructor.
            /*!
             \param lvl The logging level string.
             \param fmt The logging format string.
             */
            Logger_stream(const std::string lvl,
                    const std::string& fmt,
                    std::ostream* stream);
            //! Destructor.
            virtual ~Logger_stream();
        protected:
            virtual Logger& write_string(const std::string& msg);
            virtual Logger& write_signed_int(const int64_t msg);
            virtual Logger& write_unsigned_int(const uint64_t msg);
            virtual Logger& write_bool(const bool msg);
            virtual Logger& write_pointer(const void* msg);
            virtual void write_end();
        private:
            std::list<std::string> parts_;
            std::ostringstream buffer_;
            std::ostream* stream_;
        };
        
        //! Logger that outputs to cout.
        class Logger_cout : public Logger_stream
        {
        public:
            //! Constructor.
            /*!
             \param lvl The logging level string.
             \param fmt The logging format string.
             */
            Logger_cout(const std::string lvl,
                    const std::string& fmt);
            //! Destructor.
            virtual ~Logger_cout();
        };
        
        //! Check or set the enabled flags for a level.
        template <class Level>
        bool enabled_flag(bool* new_state = nullptr)
        {
            static bool state = true;
            if (new_state)
            {
                state = *new_state;
            }
            return state;
        }

        //! Enable logging for a level.
        template <class Level>
        void enable()
        {
            bool state = true;
            enabled_flag<Level>(&state);
        }

        //! Disable logging for a level.
        template <class Level>
        void disable()
        {
            bool state = false;
            enabled_flag<Level>(&state);
        }

        //! Create a logger that functions like a iostream.
        /*!
         \par
         This functions tests the Logging level to see if it is enabled.
         and a new logger object is created based on that result.
         \param fmt The format of the log message.
         \tparam Level The logging level of the message.
         \return A logger for passing arguments.
         */
        template <class Level>
        Logger& format(const std::string& fmt)
        {
            if (enabled_flag<Level>())
            {
                Level lvl;
                return *(new Logger_cout(lvl.name(), fmt));
            }
            else
            {
                Level lvl;
                return *(new Logger(lvl.name(), fmt));
            }
        }

        //! Write out a single log line with no arguments.
        /*!
         \par
         Creates, outputs, and automatically destroys a logger object.
         \param msg The log message.
         \tparam Level The logging level of the message.
         */
        template <class Level>
        void out(const std::string& fmt)
        {
            format<Level>(fmt).end();
        }

        template <class F>
        bool attempt(Logger& logger, F func)
        {
            try
            {
                func();
            }
            catch (const std::exception& ex)
            {
                logger.end("Unhandled Exception", ex);
                return false;
            }
            catch (std::exception* ex)
            {
                logger.end("Unhandled Exception", ex);
                delete ex;
                return false;
            }
            catch (const std::string& ex)
            {
                logger.end("Unhandled Exception", ex);
                return false;
            }
            catch (...)
            {
                logger.end("Unhandled Exception", "non-exception type");
                throw;
            }
            return true;
        }

        //! Log any exceptions encountered while executing a function.
        /*!
         \par
         This executes a function inside a try/catch block. The resulting
         exception, if understood, is output as a log message.
         \param func The function to execute. The return type is ignored
         and it cannot take any arguments.
         \tparam Level the logging level of the exception messages.
         \tparam F the type of the function.
         \return True if the function call was successful. False if an
         exception was caught.
         */
        template <class Level, class F>
        bool attempt(F func)
        {
            return attempt<F>(format<Level>("%s: %s"), func);
        }
    }; // namespace lj::log
}; // namespace lj
