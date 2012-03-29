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

#include "js/Bson.h"
#include "js/Command_language_js.h"
#include "logjamd/Connection.h"
#include "lj/Log.h"

namespace
{
    v8::Handle<v8::Value> print_to_response(const v8::Arguments& args)
    {
        v8::Handle<v8::External> external = v8::Handle<v8::External>::Cast(
                args.Data());
        lj::bson::Node* response = static_cast<lj::bson::Node*>(
                external->Value());

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

        response->push_child("output",
                lj::bson::new_string(buffer.str()));

        return v8::Undefined();
    }
    
    v8::Handle<v8::Value> change_adapt_language(const v8::Arguments& args)
    {
        v8::Handle<v8::External> external = v8::Handle<v8::External>::Cast(
                args.Data());
        lj::bson::Node* response = static_cast<lj::bson::Node*>(
                external->Value());

        v8::HandleScope handle_scope;
        v8::String::Utf8Value lang(args[0]);
        response->set_child("next_language",
                lj::bson::new_string(std::string(*lang, lang.length())));
        return v8::Undefined();
    }

    v8::Handle<v8::Value> close_connection(const v8::Arguments& args)
    {
        v8::Handle<v8::External> external = v8::Handle<v8::External>::Cast(
                args.Data());
        lj::bson::Node* response = static_cast<lj::bson::Node*>(
                external->Value());
        response->set_child("shutdown", lj::bson::new_boolean(true));
        return v8::Undefined();
    }

    v8::Handle<v8::Value> get_crypto_key(const v8::Arguments& args)
    {
        v8::Handle<v8::External> external = v8::Handle<v8::External>::Cast(
                args.Data());
        logjamd::Connection* connection = static_cast<logjamd::Connection*>(
                external->Value());

        v8::HandleScope handle_scope;
        v8::String::Utf8Value id_wrapped(args[0]);
        std::string identifier(*id_wrapped, id_wrapped.length());
        
        int sz;
        const void* data = connection->get_crypto_key(identifier, &sz);

        v8::Handle<v8::Value> result;
        if (data)
        {
            std::unique_ptr<lj::bson::Node> ptr(lj::bson::new_binary(
                    static_cast<const uint8_t*>(data),
                    sz,
                    lj::bson::Binary_type::k_bin_user_defined));
            // TODO This should be an RO version of the Bson object.
            js::Bson* key_data = new js::Bson(*ptr);
            result = js::Jesuit<js::Bson>::wrap(key_data);
        }
        else
        {
            result = v8::Undefined();
        }
        return handle_scope.Close(result);
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

    bool Command_language_js::perform(lj::bson::Node& response)
    {
        // XXX Mostly copy and paste from the V8 website.
        // Needs to be cleaned up.
        v8::HandleScope handle_scope;
        v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
        v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);
        v8::Context::Scope context_scope(context);
        configure_context(response);

        js::Bson* foo = new js::Bson();
        context->Global()->Set(v8::String::NewSymbol("foo"),
                js::Jesuit<js::Bson>::wrap(foo));
        foo->node().set_child("test", lj::bson::new_string("hello"));
        foo->node().set_child("bar", lj::bson::new_boolean(true));

        std::string cmd(lj::bson::as_string(request_->nav("command")));
        v8::Handle<v8::String> source =
                v8::String::New(cmd.c_str(), cmd.length());
        v8::Handle<v8::Script> script = v8::Script::Compile(source);
        v8::Handle<v8::Value> result = script->Run();
        context.Dispose();
        v8::String::AsciiValue ascii(result);
        response.set_child("message",
                lj::bson::new_string(*ascii));

        bool keep_alive = true;
        if (response.exists("shutdown"))
        {
            response.set_child("shutdown", NULL);
            keep_alive = false;
        }
        return keep_alive;
    }

    std::string Command_language_js::name()
    {
        return "JavaScript";
    }

    void Command_language_js::configure_context(lj::bson::Node& response)
    {
        // print
        v8::Handle<v8::FunctionTemplate> print = v8::FunctionTemplate::New(
                print_to_response,
                v8::External::New(&response));
        v8::Context::GetCurrent()->Global()->Set(
                v8::String::New("print"),
                print->GetFunction());

        // change the command language.
        v8::Handle<v8::FunctionTemplate> chlang = v8::FunctionTemplate::New(
                change_adapt_language,
                v8::External::New(&response));
        v8::Context::GetCurrent()->Global()->Set(
                v8::String::New("change_language"),
                chlang->GetFunction());

        // Exit the connection.
        v8::Handle<v8::FunctionTemplate> closeconn = v8::FunctionTemplate::New(
                close_connection,
                v8::External::New(&response));
        v8::Context::GetCurrent()->Global()->Set(
                v8::String::New("exit"),
                closeconn->GetFunction());

        // Get a crypto key.
        v8::Handle<v8::FunctionTemplate> getcrypto = v8::FunctionTemplate::New(
                get_crypto_key,
                v8::External::New(connection_));
        v8::Context::GetCurrent()->Global()->Set(
                v8::String::New("get_crypto_key"),
                getcrypto->GetFunction());
    }
};
