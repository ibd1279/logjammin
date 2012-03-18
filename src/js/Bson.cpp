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
        {0,0,0,0,0,0}
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
        std::string tmp(lj::bson::type_string(node_->type()));
        return v8::String::New(tmp.data(), tmp.size());
    }
}; // namespace js
