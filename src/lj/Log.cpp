/*!
 \file lj/Log.cpp
 \brief LJ Logger implementation.
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

#include "lj/Log.h"

#include <cstdio>
#include <iostream>
#include <memory>

lj::log::End lj::log::end;

namespace lj
{
    namespace log
    {
        Logger_stream::Logger_stream(const std::string lvl,
                const std::string& fmt,
                std::ostream* stream) :
                Logger(lvl, fmt),
                stream_(stream)
        {
            std::string tmp;
            for (auto iter = fmt.begin();
                 iter != fmt.end();
                 ++iter)
            {
                if (*iter == '%')
                {
                    ++iter;
                    if(iter == fmt.end())
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
            buffer_ << '[' << lvl << "] ";
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

        Logger_stream::~Logger_stream()
        {
        }

        Logger& Logger_stream::write_string(const std::string& msg)
        {
            if (parts_.size() > 0)
            {
                const std::string& fmt = parts_.front();
                char* buffer = new char[msg.size() + fmt.size() + 80];
                sprintf(buffer, fmt.c_str(), msg.c_str());
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

        Logger& Logger_stream::write_signed_int(const int64_t msg)
        {
            if (parts_.size() > 0)
            {
                const std::string& fmt = parts_.front();
                char* buffer = new char[512 + fmt.size()];
                sprintf(buffer, fmt.c_str(), msg);
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

        Logger& Logger_stream::write_unsigned_int(const uint64_t msg)
        {
            if (parts_.size() > 0)
            {
                const std::string& fmt = parts_.front();
                char* buffer = new char[512 + fmt.size()];
                sprintf(buffer, fmt.c_str(), msg);
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

        Logger& Logger_stream::write_bool(const bool msg)
        {
            if (parts_.size() > 0)
            {
                const std::string& fmt = parts_.front();
                char* buffer = new char[128 + fmt.size()];
                if (fmt.at(1) == 's')
                {
                    sprintf(buffer, fmt.c_str(), msg ? "true" : "false");
                }
                else
                {
                    sprintf(buffer, fmt.c_str(), msg);
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

        Logger& Logger_stream::write_pointer(const void* msg)
        {
            if (parts_.size() > 0)
            {
                const std::string& fmt = parts_.front();
                char* buffer = new char[512 + fmt.size()];
                sprintf(buffer, fmt.c_str(), msg);
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

        void Logger_stream::write_end()
        {
            for (auto iter = parts_.begin(); iter != parts_.end(); ++iter)
            {
                buffer_ << "..." << (*iter);
            }
            (*stream_) << buffer_.str() << std::endl;
            delete this;
        }

        Logger_clog::Logger_clog(const std::string lvl,
                const std::string& fmt) :
                Logger_stream(lvl, fmt, &(std::clog))
        {
        }
    }; // namespace log
}; // namespace lj

