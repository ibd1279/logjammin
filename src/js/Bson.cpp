/*!
 \file js/Bson.cpp
 \brief Logjam server Javascript Bson wrapper implementation.
 \author Jason Watson

 Copyright (c) 2012, Jason Watson
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

namespace js
{
    Jesuit<Bson>::Cache Bson::JESUIT_CACHE;
    Jesuit<Bson>::Accessors Bson::JESUIT_ACCESSORS[] = {
        JESUIT_METHOD(Bson, nullify),
        JESUIT_METHOD(Bson, path),
        JESUIT_CALL_AS(Bson, path),
        JESUIT_ACCESSOR(Bson, value),
        JESUIT_ACCESSOR(Bson, type),
        JESUIT_END
    };

    Bson::Bson() :
            node_(new lj::bson::Node())
    {
    }

    Bson::Bson(const lj::bson::Node& val) :
            node_(new lj::bson::Node(val))
    {
    }

    Bson::Bson(std::shared_ptr<lj::bson::Node>& root,
            const std::string& path) :
            node_(root, root->path(path))
    {
    }

    Bson::~Bson()
    {
    }

    lj::bson::Node& Bson::node()
    {
        return *node_;
    }

    v8::Handle<v8::Value> Bson::type(v8::Local<v8::String> prop,
                    const v8::AccessorInfo& info)
    {
        std::string tmp(lj::bson::type_string(node().type()));
        return v8::String::New(tmp.data(), tmp.size());
    }

    v8::Handle<v8::Value> Bson::nullify(const v8::Arguments& args)
    {
        node().nullify();
        return v8::Undefined();
    }

    v8::Handle<v8::Value> Bson::path(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        js::Bson* obj = new js::Bson(node_, path);
        return js::Jesuit<js::Bson>::wrap(obj);
    }

    v8::Handle<v8::Value> Bson::value(v8::Local<v8::String> prop,
                    const v8::AccessorInfo& info)
    {
        v8::Handle<v8::Value> result;
        if (lj::bson::type_is_nested(node().type()))
        {
            result = js::Jesuit<js::Bson>::wrap(this);
        }
        else if (lj::bson::Type::k_null == node().type())
        {
            result = v8::Null();
        }
        else if (lj::bson::type_is_quotable(node().type()))
        {
            std::string value = lj::bson::as_string(node());
            result = v8::String::New(value.data(), value.size());
        }
        else if (lj::bson::Type::k_int32 == node().type())
        {
            // handle int32 as an js int.
            result = v8::Integer::New(lj::bson::as_int32(node()));
        }
        else if (lj::bson::type_is_number(node().type()))
        {
            // everything else won't fit, so treat as a double. :(
            result = v8::Number::New(lj::bson::as_int64(node()));
        }
        else if (lj::bson::Type::k_boolean == node().type())
        {
            result = lj::bson::as_boolean(node()) ? v8::True() : v8::False();
        }
        else
        {
            result = v8::Undefined();
        }
        return result;
    }
}; // namespace js
