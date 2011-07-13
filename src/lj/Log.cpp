/*!
 \file lj/Log.cpp
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

#include "lj/Log.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>

lj::Log lj::Log::emergency(lj::Log::Level::k_emergency, &std::cerr);
lj::Log lj::Log::alert(lj::Log::Level::k_alert, &std::cerr);
lj::Log lj::Log::critical(lj::Log::Level::k_critical, &std::cerr);
lj::Log lj::Log::error(lj::Log::Level::k_error, &std::cerr);
lj::Log lj::Log::warning(lj::Log::Level::k_warning, &std::cerr);
lj::Log lj::Log::notice(lj::Log::Level::k_notice, &std::cerr);
lj::Log lj::Log::info(lj::Log::Level::k_info, &std::cerr);
lj::Log lj::Log::debug(lj::Log::Level::k_debug, &std::cerr);
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
                    Level level,
                    const std::string &msg) : lj::Log(level, strm), parts_(), buffer_()
        {
            std::string tmp;
            for (auto iter = msg.begin();
                 iter != msg.end();
                 ++iter)
            {
                if (*iter == '%')
                {
                    ++iter;
                    if(iter == msg.end())
                    {
                        break;
                    }
                    else if(*iter == '%')
                    {
                        tmp.push_back('%');
                    }
                    else
                    {
                        parts_.push_back(tmp);
                        tmp.clear();
                        tmp.push_back('%');
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
                    
        size_t size() const
        {
            return parts_.size();
        }
        
        std::string head() const
        {
            return parts_.front();
        }
        
        void pop()
        {
            parts_.pop_front();
        }
        
        virtual Log& write_string(const std::string& msg)
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
                buffer_ << " " << msg;
            }
            return *this;
        }
        
        virtual Log& write_signed_int(const int64_t msg)
        {
            if (parts_.size() > 0)
            {
                char *buffer = new char[512 + parts_.front().size()];
                sprintf(buffer, parts_.front().c_str(), msg);
                buffer_ << buffer;
                parts_.pop_front();
                delete[] buffer;
            }
            else
            {
                buffer_ << " " << msg;
            }
            return *this;
        }
        
        virtual Log& write_unsigned_int(const uint64_t msg)
        {
            if (parts_.size() > 0)
            {
                char *buffer = new char[512 + parts_.front().size()];
                sprintf(buffer, parts_.front().c_str(), msg);
                buffer_ << buffer;
                parts_.pop_front();
                delete[] buffer;
            }
            else
            {
                buffer_ << " " << msg;
            }
            return *this;
        }

        virtual Log& write_bool(const bool msg)
        { 
            if (parts_.size() > 0)
            {
                char *buffer = new char[128 + parts_.front().size()];
                if (parts_.front().at(1) == 's')
                {
                    sprintf(buffer, parts_.front().c_str(), msg ? "true" : "false");
                }
                else
                {
                    sprintf(buffer, parts_.front().c_str(), msg);
                }
                buffer_ << buffer;
                parts_.pop_front();
                delete[] buffer;
            }
            else
            {
                buffer_ << (msg ? " true" : " false");
            }
            return *this;
        }
        
        virtual Log& write_pointer(const void* msg)
        {
            char *buffer = new char[512 + parts_.front().size()];
            if (parts_.size() > 0)
            {
                sprintf(buffer, parts_.front().c_str(), msg);
                buffer_ << buffer;
                parts_.pop_front();
            }
            else
            {
                sprintf(buffer, " %p", msg);
                buffer_ << msg;
            }
            delete[] buffer;
            return *this;
        }
        
        virtual void write_end()
        {
            for (auto iter = parts_.begin(); iter != parts_.end(); ++iter)
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
    
    const std::string k_level_emergency_string("[EMERGENCY]   ");
    const std::string k_level_alert_string("[ALERT]       ");
    const std::string k_level_critical_string("[CRITICAL]    ");
    const std::string k_level_error_string("[ERROR]       ");
    const std::string k_level_warning_string("[WARNING]     ");
    const std::string k_level_notice_string("[NOTICE]      ");
    const std::string k_level_info_string("[INFORMATION] ");
    const std::string k_level_debug_string("[DEBUG]       ");
    
}; // namespace
    
namespace lj
{    
    const std::string& Log::level_text(const Level& level)
    {
        switch (level)
        {
            case Level::k_emergency:
                return k_level_emergency_string;
            case Level::k_alert:
                return k_level_alert_string;
            case Level::k_critical:
                return k_level_critical_string;
            case Level::k_error:
                return k_level_error_string;
            case Level::k_warning:
                return k_level_warning_string;
            case Level::k_notice:
                return k_level_notice_string;
            case Level::k_info:
                return k_level_info_string;
            default:
                return k_level_debug_string;
        }
    }
    Log& Log::for_level(const Level& level)
    {
        switch (level)
        {
            case Level::k_emergency:
                return Log::emergency;
            case Level::k_alert:
                return Log::alert;
            case Level::k_critical:
                return Log::critical;
            case Level::k_error:
                return Log::error;
            case Level::k_warning:
                return Log::warning;
            case Level::k_notice:
                return Log::notice;
            case Level::k_info:
                return Log::info;
            default:
                return Log::debug;
        }
    }
    
    Log &Log::operator()(const std::string& fmt)
    {
        if (enabled_)
        {
            return *(new Real_logger(stream_, level_, fmt));
        }
        else return *this;
    }
}; //namespace lj
