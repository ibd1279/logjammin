#pragma once
/*!
 \file lj/Log.h
 \brief LJ Log header.
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

#include "lj/Uuid.h"

#include <cstdint>
#include <exception>
#include <list>
#include <map>
#include <sstream>
#include <string>

namespace lj
{
    /*!
     \brief Base class for Logging Levels.
     \since 1.0
     */
    class LogLevel
    {
    private:
        const std::string name_;
        const int level_;
    public:
        /*!
         \brief Create a new logging level.
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
        virtual ~LogLevel() = default;

        /*!
         \brief Get the logging level name.
         \return The logging level name.
         */
        const std::string& name() const
        {
            return name_;
        }

        /*!
         \brief Get the logging level number.
         \return The logging level number.
         */
        const int level() const
        {
            return level_;
        }
    }; // class lj::LogLevel

    /*!
     \brief Emergency Logging Level
     \since 1.0
     */
    struct Emergency : public LogLevel
    {
        Emergency() : LogLevel("EMERGENCY", 0)
        {
        }
    }; // struct lj::Emergency

    /*!
     \brief Alert Logging Level
     \since 1.0
     */
    struct Alert : public LogLevel
    {
        Alert() : LogLevel("ALERT", 1)
        {
        }
    }; // struct lj::Alert

    /*!
     \brief Critical Logging Level
     \since 1.0
     */
    struct Critical : public LogLevel
    {
        Critical() : LogLevel("CRITICAL", 2)
        {
        }
    }; // struct lj::Critical

    /*!
     \brief Error Logging Level
     \since 1.0
     */
    struct Error : public LogLevel
    {
        Error() : LogLevel("ERROR", 3)
        {
        }
    }; // struct lj::Error

    /*!
     \brief Warning Logging Level
     \since 1.0
     */
    struct Warning : public LogLevel
    {
        Warning() : LogLevel("WARNING", 4)
        {
        }
    }; // struct lj::Warning

    /*!
     \brief Notice Logging Level
     \since 1.0
     */
    struct Notice : public LogLevel
    {
        Notice() : LogLevel("NOTICE", 5)
        {
        }
    }; // struct lj::Notice

    /*!
     \brief Info Logging Level
     \since 1.0
     */
    struct Info : public LogLevel
    {
        Info() : LogLevel("INFORMATION", 6)
        {
        }
    }; // struct lj::Info

    /*!
     \brief Debug Logging Level
     \since 1.0
     */
    struct Debug : public LogLevel
    {
        Debug() : LogLevel("DEBUG", 7)
        {
        }
    }; // struct lj::Debug

    namespace log
    {
        /*!
         \brief type for closing a logger object.
         \since 1.0
         */
        struct End
        {
        }; // struct lj::log::End

        extern End end; //!< The end object, for closing a logger object.

        /*!
         \brief Logger Base class.

         Provides a default implementation for logging, which is a no-op.
         Specific implementations are expected to override the protected methods
         of this logger to write logging information somewhere useful.

         \since 1.0
         */
        class Logger
        {
        public:
            /*!
             \brief Constructor used by the Log template.
             \param lvl The logging level string.
             \param fmt The log formatting string.
             \sa lj::log::format()
             */
            Logger(const std::string lvl,
                    const std::string& fmt)
            {
            }

            //! destructor.
            virtual ~Logger() = default;
        protected:
            /*!
             \brief Write a string to the output stream.
             \param msg The message to write.
             \return The current Log.
             */
            virtual Logger& write_string(const std::string& msg)
            {
                return *this;
            }

            /*!
             \brief Write a signed integer to the output stream.
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_signed_int(const int64_t msg)
            {
                return *this;
            }

            /*!
             \brief Write an unsigned integer to the output stream.
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_unsigned_int(const uint64_t msg)
            {
                return *this;
            }

            /*!
             \brief Write a bool to the output stream.
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_bool(const bool msg)
            {
                return *this;
            }

            /*!
             \brief Write a pointer address to the output stream.
             \param msg The message to write to the output.
             \return The current Log.
             */
            virtual Logger& write_pointer(const void* msg)
            {
                return *this;
            }

            /*!
             \brief End the current log message.
             \return The current Log.
             */
            virtual void write_end()
            {
            }

        public:
            /*!
             \brief Log a value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const std::string& msg) { return write_string(msg); };

            /*!
             \brief Log a value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const char* msg) { return write_string(msg == 0 ? std::string("NULL") : std::string(msg)); };
            
            /*!
             \brief Log a size_t value.

             For some reason, logging the size from the collection classes
             doesn't fall under one of eight other integer writing overloads
             below. So this one is to take care of those. Oh the joys of
             overloading.

             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const size_t msg) {return write_unsigned_int(msg); };

            /*!
             \brief Log a signed int 64 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int64_t msg) { return write_signed_int(msg); };

            /*!
             \brief Log an unsigned int 64 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint64_t msg) { return write_unsigned_int(msg); };

            /*!
             \brief Log a signed int 32 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int32_t msg) { return write_signed_int(msg); };

            /*!
             \brief Log an unsigned int 32 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint32_t msg) { return write_unsigned_int(msg); };

            /*!
             \brief Log a signed int 16 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int16_t msg) { return write_signed_int(msg); };

            /*!
             \brief Log an unsigned int 16 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint16_t msg) { return write_unsigned_int(msg); };

            /*!
             \brief Log a signed int 8 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const int8_t msg) { return write_signed_int(msg); };

            /*!
             \brief Log an unsigned int 8 value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const uint8_t msg) { return write_unsigned_int(msg); };

            /*!
             \brief Log a boolean value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const bool msg) { return write_bool(msg); };

            /*!
             \brief Log a pointer value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const void* msg) { return write_pointer(msg); };

            /*!
             \brief Log a unique id value.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline Logger& operator<<(const Uuid& msg) { return write_string(msg.str()); };

            /*!
             \brief Log an exception.
             \param ex The exception to log
             \return The current Log.
             */
            inline Logger& operator<<(const std::exception& ex) { return write_string(ex.what()); };

            /*!
             \brief Log a map
             \param m The map.
             \tparam K the key type
             \tparam V the value type.
             \return The current Log.
             */
            template <class K, class V>
            inline Logger& operator<<(const std::map<K, V> m)
            {
                for (const typename std::map<K, V>::value_type& item : m)
                {
                    (*this) << "[" << item.first << "=" << item.second << "]";
                }
                return *this;
            }

            /*!
             \brief Close the logger.
             \param msg The message to write to the output.
             \return The current Log.
             */
            inline void operator<<(const End &msg) { write_end(); };

            /*!
             \brief Helper function to make closing a logger easier.

             This is the tailing call of the variadic template.
             */
            void end()
            {
                (*this) << lj::log::end;
            }

            /*!
             \brief Helper function to make logging a single function call.

             This is to allow logging to be as condense as possible. It is normally
             used as follows:
             \code
             lj::log::format<lj::Info>("Something Happened: %d - %s").end(errno, strerror(errno));
             \endcode

             Depends on the C++11 variadic templates.
             \param a0 The first argument to write.
             \param ...args The rest of the args.
             \tparam A0 The first argument type.
             \tparam ...Args The rest of the arg types.
             */
            template <class A0, class ...Args>
            void end(const A0& a0, const Args& ...args)
            {
                (*this) << a0;
                end(args...);
            }
        }; // class lj::log::Logger

        /*!
         \brief Included logger for writing to an std::ostream.
         \since 1.0
         */
        class Logger_stream : public Logger
        {
        public:
            /*!
             \brief Constructor.
             \param lvl The logging level string.
             \param fmt The logging format string.
             \param stream The output stream to use.
             */
            Logger_stream(const std::string lvl,
                    const std::string& fmt,
                    std::ostream* stream);
            //! Destructor.
            virtual ~Logger_stream();
        protected:
            virtual Logger& write_string(const std::string& msg) override;
            virtual Logger& write_signed_int(const int64_t msg) override;
            virtual Logger& write_unsigned_int(const uint64_t msg) override;
            virtual Logger& write_bool(const bool msg) override;
            virtual Logger& write_pointer(const void* msg) override;
            virtual void write_end() override;
        private:
            std::list<std::string> parts_;
            std::ostringstream buffer_;
            std::ostream* stream_;
        }; // class lj::log::Logger_stream

        /*!
         \brief Logger that outputs to std::cout.
         \since 1.0
         */
        class Logger_clog : public Logger_stream
        {
        public:
            /*!
             \brief Constructor.
             \param lvl The logging level string.
             \param fmt The logging format string.
             */
            Logger_clog(const std::string lvl,
                    const std::string& fmt);
            //! Destructor.
            virtual ~Logger_clog() = default;
        }; // class lj::log::Logger_clog

        /*!
         \brief Check or set the enabled flags for a level.

         Must usages will be to check if a given logging level is enabled.
         however, if needed, this can be used to set the level of logging based
         on a parameter.

         \param new_state Optional parameter used to change the state of the
         logging level. This must be a pointer to a boolean object.
         \tparam Level The logging level to test.
         \return The state of the logging level when this method has returned.
         */
        template <class Level>
        bool enabled_flag(const bool* const new_state = nullptr)
        {
            static bool state = true;
            if (new_state)
            {
                state = *new_state;
            }
            return state;
        }

        /*!
         \brief Enable logging for a level.

         Helper method for enabling a logging level.
         \tparam Level The logging level to enable.
         */
        template <class Level>
        void enable()
        {
            bool state = true;
            enabled_flag<Level>(&state);
        }

        /*!
         \brief Disable logging for a level.

         Helper method for disabling a logging level.
         \tparam Level The logging level to disable.
         */
        template <class Level>
        void disable()
        {
            bool state = false;
            enabled_flag<Level>(&state);
        }

        /*!
         \brief Create a logger that is expecting parameters.

         This functions tests the Logging level to see if it is enabled.
         and a new logger object is created based on that result. At the moment,
         it is hard coded to always used the lj::log::Logger_clog.

         \todo The logger type should be able to be driven by some form of
         configuration.
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
                return *(new Logger_clog(lvl.name(), fmt));
            }
            else
            {
                Level lvl;
                return *(new Logger(lvl.name(), fmt));
            }
        }

        /*!
         \brief Write out a single log line with no arguments.

         Creates, outputs, and automatically destroys a logger object.
         \param fmt The log message.
         \tparam Level The logging level of the message.
         */
        template <class Level>
        void out(const std::string& fmt)
        {
            format<Level>(fmt).end();
        }

        /*!
         \brief Wrap a function call with logging.

         Attempt invokes the provided function call inside a try block. On
         On failure, it makes a best effort to catch the exception and log the
         value.
         \tparam F The function pointer type.
         \param logger The logger to use for exception reporting.
         \param func The function to call.
         */
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

        /*!
         \brief Log any exceptions encountered while executing a function.

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
