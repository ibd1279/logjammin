#include "logjamd/Response.h"
/*!
 \file Response.cpp
 \brief Logjamd server response implementation.
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

namespace logjamd
{
    namespace response
    {
        lj::bson::Node new_empty(const logjam::Stage& generator)
        {
            lj::bson::Node n;
            n.set_child("stage", lj::bson::new_string(generator.name()));
            n.set_child("success", lj::bson::new_boolean(true));
            n.set_child("message", lj::bson::new_string("ok"));
            return n;
        }

        lj::bson::Node new_error(const logjam::Stage& generator,
                const std::string& msg)
        {
            lj::bson::Node n;
            n.set_child("stage", lj::bson::new_string(generator.name()));
            n.set_child("success", lj::bson::new_boolean(false));
            n.set_child("message", lj::bson::new_string(msg));
            return n;
        }
    }; // namespace logjamd::response
}; // namespace logjamd
