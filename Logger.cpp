/*
 \file Logger.cpp
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

#include "Logger.h"
#include <cstdio>
#include <iostream>
#include <list>
#include <sstream>

logjam::Log logjam::Log::emergency(&std::cerr, logjam::Log::EMERGENCY);
logjam::Log logjam::Log::alert(&std::cerr, logjam::Log::ALERT);
logjam::Log logjam::Log::critical(&std::cerr, logjam::Log::CRITICAL);
logjam::Log logjam::Log::error(&std::cerr, logjam::Log::ERROR);
logjam::Log logjam::Log::warning(&std::cerr, logjam::Log::WARNING);
logjam::Log logjam::Log::notice(&std::cerr, logjam::Log::DEBUG);
logjam::Log logjam::Log::info(&std::cerr, logjam::Log::INFO);
logjam::Log logjam::Log::debug(&std::cerr, logjam::Log::DEBUG);
logjam::Log::End logjam::Log::end;

namespace logjam {
    namespace {
        class RealLogger : public Log {
            std::list<std::string> _parts;
            std::ostringstream _buffer;
        public:
            RealLogger(std::ostream *strm, event_level level, const std::string &msg) : Log(strm, level), _parts(), _buffer() {
                std::string tmp;
                for(std::string::const_iterator iter = msg.begin();
                    iter != msg.end();
                    ++iter) {
                    if(*iter == '%') {
                        _parts.push_back(tmp);
                        tmp.clear();
                        tmp.push_back((*iter++));
                        if(iter == msg.end())
                            break;
                    }
                    tmp.push_back(*iter);
                }
                if(tmp.size() > 0)
                    _parts.push_back(tmp);
                _buffer << "[" << lvl_txt() << "] ";
                if(_parts.size() < 2) {
                    _buffer << tmp << std::endl;
                } else {
                    _buffer << _parts.front();
                    _parts.pop_front();
                }
            }
            virtual Log &operator<<(const std::string &msg) {
                if(_parts.size() > 0) {
                    char *buffer = new char[msg.size() + _parts.front().size()];
                    sprintf(buffer, _parts.front().c_str(), msg.c_str());
                    _buffer << buffer;
                    _parts.pop_front();
                    delete[] buffer;
                } else {
                    _buffer << msg;
                }
                return *this;
            }
            virtual Log &operator<<(const char *msg) {
                if(_parts.size() > 0) {
                    char *buffer = new char[strlen(msg) + _parts.front().size()];
                    std::cout << _parts.front() << std::endl;
                    sprintf(buffer, _parts.front().c_str(), msg);
                    _buffer << buffer;
                    _parts.pop_front();
                    delete[] buffer;
                } else {
                    _buffer << msg;
                }
                return *this;
            }
            virtual void operator<<(const Log::End &msg) {
                for(std::list<std::string>::const_iterator iter = _parts.begin();
                    iter != _parts.end();
                    ++iter) {
                    _buffer << "..." << (*iter);
                }
                (*_strm) << _buffer.str() << std::endl;
                delete this;
            }
        };
    };

    Log &Log::operator()(const std::string &m) {
        if(_enabled) {
            return *(new RealLogger(_strm, _level, m));
        }
        else return *this;
    }
};
