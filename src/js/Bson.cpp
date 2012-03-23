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
        JESUIT_ACCESSOR(Bson, type),
        JESUIT_METHOD(Bson, nullify),
        JESUIT_METHOD(Bson, clone),
        JESUIT_METHOD(Bson, path),
        JESUIT_CALL_AS(Bson, path),
        JESUIT_ACCESSOR(Bson, value),
        JESUIT_METHOD(Bson, setNull),
        JESUIT_METHOD(Bson, setDocument),
        JESUIT_METHOD(Bson, setArray),
        JESUIT_METHOD(Bson, setBoolean),
        JESUIT_METHOD(Bson, setString),
        JESUIT_METHOD(Bson, setInt32),
        JESUIT_METHOD(Bson, setInt64),
        JESUIT_METHOD(Bson, setUuid),
        JESUIT_METHOD(Bson, toString),
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

    v8::Handle<v8::Value> Bson::clone(const v8::Arguments& args)
    {
        js::Bson* obj = new js::Bson(node());
        return js::Jesuit<js::Bson>::wrap(obj);
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

    v8::Handle<v8::Value> Bson::setNull(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        node().set_child(path, lj::bson::new_null());
        return js::Jesuit<js::Bson>::wrap(this);
    }

    v8::Handle<v8::Value> Bson::setDocument(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        if (args.Length() == 2)
        {
            // This copies the value into the current document.
            // It feels like a link would work better, but I would
            // need to do some serious playing with the shared_ptr
            // to prevent accidental GC
            // TODO this should do something nice with plain objects
            // using named fields.
            v8::Handle<v8::Object> tmp = v8::Handle<v8::Object>::Cast(args[1]);
            js::Bson* obj = js::Jesuit<js::Bson>::unwrap(tmp);
            node().set_child(path, new lj::bson::Node(obj->node()));
        }
        else
        {
            node().set_child(path, new lj::bson::Node());
        }
        return js::Jesuit<js::Bson>::wrap(this);
    }

    v8::Handle<v8::Value> Bson::setArray(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        if (args.Length() == 2)
        {
            // This copies the value into the current document.
            // It feels like a link would work better, but I would
            // need to do some serious playing with the shared_ptr
            // to prevent accidental GC
            // by definition, this would work identically to the set
            // document above, which is accidental and should be fixed.
            // TODO this should do something nice with js array objects.
            v8::Handle<v8::Object> tmp = v8::Handle<v8::Object>::Cast(args[1]);
            js::Bson* obj = js::Jesuit<js::Bson>::unwrap(tmp);
            node().set_child(path, new lj::bson::Node(obj->node()));
        }
        else
        {
            node().set_child(path, lj::bson::new_array());
        }
        return js::Jesuit<js::Bson>::wrap(this);
    }

    v8::Handle<v8::Value> Bson::setBoolean(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        node().set_child(path, lj::bson::new_boolean(args[1]->IsTrue()));
        return js::Jesuit<js::Bson>::wrap(this);
    }

    v8::Handle<v8::Value> Bson::setString(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        v8::String::Utf8Value val(args[1]);
        std::string value(*str, str.length());
        node().set_child(path, lj::bson::new_string(value));
        return js::Jesuit<js::Bson>::wrap(this);
    }

    v8::Handle<v8::Value> Bson::setInt32(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        v8::Handle<v8::Integer> val = v8::Handle<v8::Integer>::Cast(args[1]);
        node().set_child(path, lj::bson::new_int32(val->Value()));
        return js::Jesuit<js::Bson>::wrap(this);
    }

    v8::Handle<v8::Value> Bson::setInt64(const v8::Arguments& args)
    {
        v8::String::Utf8Value str(args[0]);
        std::string path(*str, str.length());
        v8::Handle<v8::Integer> val = v8::Handle<v8::Integer>::Cast(args[1]);
        node().set_child(path, lj::bson::new_int64(val->Value()));
        return js::Jesuit<js::Bson>::wrap(this);
    }

    v8::Handle<v8::Value> Bson::setUuid(const v8::Arguments& args)
    {
        return v8::Undefined();
    }

    v8::Handle<v8::Value> Bson::toString(const v8::Arguments& args)
    {
        v8::Handle<v8::String> result;
        std::string value = lj::bson::as_string(node());
        result = v8::String::New(value.data(), value.size());
        return result;
    }
}; // namespace js
