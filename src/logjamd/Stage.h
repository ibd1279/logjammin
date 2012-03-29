#pragma once
/*!
 \file Stage.h
 \brief Logjam server stage abstract base definition.
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

#include "lj/Bson.h"
#include "lj/Log.h"
#include <memory>
#include <string>

namespace logjamd
{
    class Connection;
    class Stage
    {
    public:
        Stage(logjamd::Connection* connection) : connection_(connection)
        {
        }
        virtual ~Stage()
        {
        }
        virtual Stage* logic() = 0;
        virtual std::string name() = 0;
        virtual logjamd::Connection* conn()
        {
            return connection_;
        }
    protected:
        virtual lj::bson::Node empty_response()
        {
            lj::bson::Node n;
            n.set_child("stage", lj::bson::new_string(name()));
            n.set_child("success", lj::bson::new_boolean(true));
            n.set_child("message", lj::bson::new_string("ok"));
            return n;
        }
        virtual lj::bson::Node error_response(const std::string& msg)
        {
            lj::bson::Node n;
            n.set_child("stage", lj::bson::new_string(name()));
            n.set_child("success", lj::bson::new_boolean(false));
            n.set_child("message", lj::bson::new_string(msg));
            return n;
        }
        virtual lj::log::Logger& log(const std::string& fmt)
        {
            std::string real_fmt("%s: ");
            real_fmt.append(fmt);
            return lj::log::format<lj::Debug>(real_fmt) << name();
        }
    private:
        logjamd::Connection* connection_;
    };
};
