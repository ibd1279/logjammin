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

#include <iomanip>
#include <ostream>
#include <string>

namespace lj {
    class Log {
    public:
        enum event_level {
            EMERGENCY,
            ALERT,
            CRITICAL,
            ERROR,
            WARNING,
            NOTICE,
            INFO,
            DEBUG
        };
        struct End {
        };
    protected:
        event_level _level;
        bool _enabled;
        std::ostream *_strm;
        std::string lvl_txt() const {
            switch(_level) {
                case EMERGENCY:
                    return std::string("EMERGENCY");
                case ALERT:
                    return std::string("ALERT");
                case CRITICAL:
                    return std::string("CRITICAL");
                case ERROR:
                    return std::string("ERROR");
                case WARNING:
                    return std::string("WARNING");
                case NOTICE:
                    return std::string("NOTICE");
                case INFO:
                    return std::string("INFORMATION");
                default:
                    return std::string("DEBUG");
            }
        }
    protected:
    public:
        Log(std::ostream *s, event_level lvl) : _level(lvl), _enabled(true), _strm(s) { }
        virtual ~Log() {}
        Log &disable() { _enabled  = false; return *this; }
        Log &enable() { _enabled = true; return *this; }
        Log &operator()(const std::string &m);
        virtual Log &operator<<(const std::string &msg) { return *this; };
        virtual Log &operator<<(const char *msg) { return *this; };
        virtual Log &operator<<(long long msg) { return *this; };
        virtual Log &operator<<(unsigned long long msg) { return *this; };
        virtual Log &operator<<(long msg) { return *this; };
        virtual Log &operator<<(unsigned long msg) { return *this; };
        virtual Log &operator<<(int msg) { return *this; };
        virtual Log &operator<<(unsigned int msg) { return *this; };
        virtual Log &operator<<(short msg) { return *this; };
        virtual Log &operator<<(unsigned short msg) { return *this; };
        virtual Log &operator<<(char msg) { return *this; };
        virtual Log &operator<<(unsigned char msg) { return *this; };
        virtual Log &operator<<(bool msg) { return *this; };
        virtual void operator<<(const End &msg) { };
        static Log emergency, alert, critical, error, warning, notice, info, debug;
        static End end;
    };
};