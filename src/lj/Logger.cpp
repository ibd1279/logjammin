/*!
 \file Logger.cpp
 \brief LJ Logger implementation.
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

#include "lj/Logger.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>

lj::Log lj::Log::emergency(lj::Log::level_emergency, &std::cerr);
lj::Log lj::Log::alert(lj::Log::level_alert, &std::cerr);
lj::Log lj::Log::critical(lj::Log::level_critical, &std::cerr);
lj::Log lj::Log::error(lj::Log::level_error, &std::cerr);
lj::Log lj::Log::warning(lj::Log::level_warning, &std::cerr);
lj::Log lj::Log::notice(lj::Log::level_notice, &std::cerr);
lj::Log lj::Log::info(lj::Log::level_info, &std::cerr);
lj::Log lj::Log::debug(lj::Log::level_debug, &std::cerr);
lj::Log::End lj::Log::end;

namespace
{
    //! Real logging class.
    /*!
     \par
     This class is instantiated by the lj:Log class when a logger is enabled.
     Where the lj::Log class does nothing with the \c lj::Log::operator<<()
     calls, this class tries to construct a log message out of them.
     \warning You should not instantiate this class on your own.
     \sa lj::Log for static accessible logger objects.
     \author Jason Watson
     \version 1.0
     \date April 16, 2010
     */
    class Real_logger : public lj::Log {
    public:
        Real_logger(std::ostream *strm,
                    Event_level level,
                    const std::string &msg) : lj::Log(level, strm), parts_(), buffer_()
        {
            std::string tmp;
            for (std::string::const_iterator iter = msg.begin();
                 iter != msg.end();
                 ++iter)
            {
                if (*iter == '%')
                {
                    parts_.push_back(tmp);
                    tmp.clear();
                    tmp.push_back((*iter++));
                    if(iter == msg.end())
                    {
                        break;
                    }
                }
                tmp.push_back(*iter);
            }
            if (tmp.size() > 0)
            {
                parts_.push_back(tmp);
            }
            buffer_ << level_text(level_);
            if (parts_.size() < 2)
            {
                parts_.clear();
                buffer_ << tmp;
            }
            else
            {
                buffer_ << parts_.front();
                parts_.pop_front();
            }
        }
        
        virtual Log &operator<<(const std::string &msg)
        {
            if (parts_.size() > 0)
            {
                char *buffer = new char[msg.size() + parts_.front().size()];
                sprintf(buffer, parts_.front().c_str(), msg.c_str());
                buffer_ << buffer;
                parts_.pop_front();
                delete[] buffer;
            }
            else
            {
                buffer_ << msg;
            }
            return *this;
        }
        
        virtual Log &operator<<(const char *msg)
        {
            if (0 == msg) 
            {
                msg = "NULL";
            }
            
            if (parts_.size() > 0)
            {
                char *buffer = new char[strlen(msg) + parts_.front().size()];
                sprintf(buffer, parts_.front().c_str(), msg);
                buffer_ << buffer;
                parts_.pop_front();
                delete[] buffer;
            }
            else
            {
                buffer_ << msg;
            }
            return *this;
        }
        
        void write_number(long long msg)
        {
            if (parts_.size() > 0)
            {
                char *buffer = new char[64 + parts_.front().size()];
                sprintf(buffer, parts_.front().c_str(), msg);
                buffer_ << buffer;
                parts_.pop_front();
                delete[] buffer;
            } else {
                buffer_ << msg;
            }
        }
        Log &operator<<(long long msg)
        {
            write_number(msg);
            return *this;
        }
        Log &operator<<(unsigned long long msg)
        {
            write_number(static_cast<long long>(msg));
            return *this;
        }
        Log &operator<<(long msg)
        {
            write_number(msg);
            return *this;
        }
        Log &operator<<(unsigned long msg)
        {
            write_number(static_cast<long long>(msg));
            return *this;
        }
        Log &operator<<(int msg)
        {
            write_number(msg);
            return *this;
        }
        Log &operator<<(unsigned int msg)
        {
            write_number(static_cast<long long>(msg));
            return *this;
        }
        Log &operator<<(short msg)
        {
            write_number(msg);
            return *this;
        }
        Log &operator<<(unsigned short msg)
        {
            write_number(static_cast<long long>(msg));
            return *this;
        }
        Log &operator<<(char msg)
        {
            write_number(msg);
            return *this;
        }
        Log &operator<<(unsigned char msg)
        {
            write_number(static_cast<long long>(msg));
            return *this;
        }
        Log &operator<<(bool msg)
        { 
            if (parts_.size() > 0)
            {
                char *buffer = new char[6 + parts_.front().size()];
                sprintf(buffer, parts_.front().c_str(), msg ? "true" : "false");
                buffer_ << buffer;
                parts_.pop_front();
                delete[] buffer;
            }
            else
            {
                buffer_ << msg;
            }
            return *this;
        }
        virtual Log &operator<<(void* msg) {
            char *buffer = new char[64 + parts_.front().size()];
            if (parts_.size() > 0)
            {
                sprintf(buffer, parts_.front().c_str(), msg);
                buffer_ << buffer;
                parts_.pop_front();
            } else {
                sprintf(buffer, "%p", msg);
                buffer_ << msg;
            }
            delete[] buffer;
            return *this;
        }
        virtual void operator<<(const Log::End &msg)
        {
            for(std::list<std::string>::const_iterator iter = parts_.begin();
                iter != parts_.end();
                ++iter)
            {
                buffer_ << "..." << (*iter);
            }
            (*stream_) << buffer_.str() << std::endl;
            delete this;
        }
    private:
        std::list<std::string> parts_;
        std::ostringstream buffer_;
    };
}; // namespace
    
namespace lj
{    
    std::string Log::level_text(const Event_level level)
    {
        switch(level)
        {
            case level_emergency:
                return std::string("[EMERGENCY]   ");
            case level_alert:
                return std::string("[ALERT]       ");
            case level_critical:
                return std::string("[CRITICAL]    ");
            case level_error:
                return std::string("[ERROR]       ");
            case level_warning:
                return std::string("[WARNING]     ");
            case level_notice:
                return std::string("[NOTICE]      ");
            case level_info:
                return std::string("[INFORMATION] ");
            default:
                return std::string("[DEBUG]       ");
        }
    }
    
    void Log::operator()(const std::string &fmt, ...)
    {
        if (enabled_)
        {
            va_list vl;
            va_start(vl, fmt);
            char *ptr = new char[(fmt.size() * 2) + 64];
            vsprintf(ptr, fmt.c_str(), vl);
            va_end(vl);
            
            Real_logger* logger = new Real_logger(stream_, level_, std::string(ptr));
            (*logger) << end;
            
            delete[] ptr;
        }
    }
    
    Log &Log::log(const std::string &fmt)
    {
        if(enabled_)
        {
            return *(new Real_logger(stream_, level_, fmt));
        }
        else return *this;
    }
}; //namespace lj
