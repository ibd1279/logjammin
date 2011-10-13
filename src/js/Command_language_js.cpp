/*!
 \file lua/Command_language_lua.cpp
 \brief Logjam server networking header.
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

#include "logjamd/Connection.h"
#include "js/Command_language_js.h"

namespace
{
    v8::Handle<v8::Value> print_to_response(const v8::Arguments& args)
    {
        std::ostringstream buffer;
        for(int h = 0; args.Length() > h; ++h)
        {
            v8::HandleScope handle_scope;
            v8::String::Utf8Value str(args[h]);
            if (0 < h)
            {
                buffer << "\t";
            }
            buffer << std::string(*str, str.length());
        }
        // add the buffer to the response object here.
        return v8::Undefined();
    }

    v8::Persistent<v8::Context> allocate_context()
    {
        v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
        global->Set(v8::String::New("print"),
                v8::FunctionTemplate::New(print_to_response));
        return v8::Context::New(NULL, global);
    }
}; // namespace (anonymous)

namespace js
{
    Command_language_js::Command_language_js(logjamd::Connection* conn,
            lj::bson::Node* req) :
            connection_(conn),
            request_(req)
    {
    }

    Command_language_js::~Command_language_js()
    {
    }

    void Command_language_js::perform(lj::bson::Node& response)
    {
        // XXX Mostly copy and paste from the V8 website.
        // Needs to be cleaned up.
        v8::HandleScope handle_scope;
        v8::Persistent<v8::Context> context(allocate_context());
        v8::Context::Scope context_scope(context);
        std::string cmd(lj::bson::as_string(request_->nav("command")));
        v8::Handle<v8::String> source =
                v8::String::New(cmd.c_str(), cmd.length());
        v8::Handle<v8::Script> script = v8::Script::Compile(source);
        v8::Handle<v8::Value> result = script->Run();
        context.Dispose();
        v8::String::AsciiValue ascii(result);
        response.set_child("message",
                lj::bson::new_string(*ascii));
    }

    std::string Command_language_js::name()
    {
        return "JavaScript";
    }
};
