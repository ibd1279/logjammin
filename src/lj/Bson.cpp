/*!
 \file Bson.cpp
 \brief LJ Bson implementation.
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
#include "lj/Base64.h"
#include "lj/Logger.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <list>
#include <sstream>

namespace lj {
    namespace {
        //! escape a string.
        std::string escape(const std::string &val) {
            std::string r;
            for(std::string::const_iterator iter = val.begin(); iter != val.end(); ++iter) {
                char c = *iter;
                if(c == '\\' || c == '"')
                    r.push_back('\\');
                else if(c == '\n')
                    r.append("\\n\\");
                r.push_back(c);
            }
            return r;
        }
        
        void subdocument(Bson::Type parent_t, Bson& node, const char* value) {
            // treat it as a char * for pointer math reasons.
            const char *ptr = value;
            
            // calculate the end address.
            long sz = 0;
            memcpy(&sz, ptr, 4);
            const char *end = ptr + sz - 1;
            ptr += 4;
            
            if(sz == 5) {
                return;
            }
            
            // loop while the pointer is shorter 
            while(ptr < end) {
                // child type.
                unsigned char t;
                memcpy(&t, ptr++, 1);
                
                // child name.
                std::string name(ptr);
                ptr += name.size() + 1;
                
                // child node.
                Bson* new_child = new Bson(static_cast<Bson::Type>(t), ptr);
                if (Bson::k_document == parent_t)
                {
                    node.set_child(lj::bson_escape_path(name), new_child);
                }
                else if(Bson::k_array == parent_t)
                {
                    node.push_child("", new_child);
                }
                
                ptr += new_child->size();
            }
        }
        
        const std::string k_bson_type_string_string("string");
        const std::string k_bson_type_string_int32("int32");
        const std::string k_bson_type_string_double("double");
        const std::string k_bson_type_string_int64("int64");
        const std::string k_bson_type_string_timestamp("timestamp");
        const std::string k_bson_type_string_boolean("boolean");
        const std::string k_bson_type_string_null("null");
        const std::string k_bson_type_string_document("document");
        const std::string k_bson_type_string_binary_document("binary-document");
        const std::string k_bson_type_string_binary("binary");
        const std::string k_bson_type_string_array("array");
        const std::string k_bson_type_string_unknown("unknown");
        
        const std::string k_bson_binary_type_string_function("function");
        const std::string k_bson_binary_type_string_binary("binary");
        const std::string k_bson_binary_type_string_uuid("uuid");
        const std::string k_bson_binary_type_string_md5("md5");
        const std::string k_bson_binary_type_string_user_defined("user-defined");
    };
    
    const std::string& bson_type_string(const Bson::Type t)
    {
        switch (t)
        {
            case Bson::k_string:
                return k_bson_type_string_string;
            case Bson::k_binary:
                return k_bson_type_string_binary;
            case Bson::k_int32:
                return k_bson_type_string_int32;
            case Bson::k_double:
                return k_bson_type_string_double;
            case Bson::k_int64:
                return k_bson_type_string_int64;
            case Bson::k_timestamp:
                return k_bson_type_string_timestamp;
            case Bson::k_boolean:
                return k_bson_type_string_boolean;
            case Bson::k_null:
                return k_bson_type_string_null;
            case Bson::k_document:
                return k_bson_type_string_document;
            case Bson::k_binary_document:
                return k_bson_type_string_binary_document;
            case Bson::k_array:
                return k_bson_type_string_array;
            default:
                break;
        }
        return k_bson_type_string_unknown;
    }
    
    const std::string& bson_binary_type_string(const Bson::Binary_type subtype)
    {
        switch (subtype)
        {
            case Bson::k_bin_function:
                return k_bson_binary_type_string_function;
            case Bson::k_bin_binary:
                return k_bson_binary_type_string_binary;
            case Bson::k_bin_uuid:
                return k_bson_binary_type_string_uuid;
            case Bson::k_bin_md5:
                return k_bson_binary_type_string_md5;
            case Bson::k_bin_user_defined:
                return k_bson_binary_type_string_user_defined;
            default:
                break;
        }
        return k_bson_type_string_unknown;
    }
    
    size_t bson_type_min_size(const Bson::Type t)
    {
        switch (t)
        {
            case Bson::k_null:
                return 0;
            case Bson::k_boolean:
                return 1;
            case Bson::k_int32:
                return 4;
            case Bson::k_string:
            case Bson::k_binary:
            case Bson::k_binary_document:
            case Bson::k_document:
            case Bson::k_array:
                return 5;
            case Bson::k_timestamp:
            case Bson::k_int64:
            case Bson::k_double:
                return 8;
            default:
                break;
        }
        return 5;
    }
    
    //=====================================================================
    // Bson ctor/dtor
    //=====================================================================
    
    Bson::Bson() : child_map_(), last_child_(0), value_(0), type_(Bson::k_document)
    {
    }
    
    Bson::Bson(const Bson::Type t, const char *v) : child_map_(), last_child_(0), value_(0), type_(Bson::k_document)
    {
        set_value(t, v);
    }
    
    Bson::Bson(const Bson &o) : child_map_(), last_child_(o.last_child_), value_(0), type_(o.type_)
    {
        copy_from(o);
    }
    
    Bson::~Bson()
    {
        if (value_)
        {
            delete[] value_;
        }
        for (Linked_map<std::string, Bson*>::iterator iter = child_map_.begin();
             child_map_.end() != iter;
             ++iter)
        {
            delete iter->second;
        }
    }
    
    void Bson::set_value(const Bson::Type t, const char* v)
    {
        // assume the type may have changed.
        char* old = NULL;
        if (bson_type_is_nested(type()))
        {
            for (Linked_map<std::string, Bson*>::iterator iter = child_map_.begin();
                 child_map_.end() != iter;
                 ++iter)
            {
                delete iter->second;
            }
            child_map_.clear();
            value_ = NULL;
            last_child_ = 0;
        }
        else
        {
            old = value_;
            value_ = NULL;
        }
        
        // process the void pointer provided based on the provided type.
        type_ = t;
        if (v)
        {
            long long sz = 0;
            switch (type_)
            {
                case Bson::k_string:
                    memcpy(&sz, v, 4);
                    value_ = new char[sz + 4];
                    memcpy(value_, v, sz + 4);
                    break;
                case Bson::k_binary:
                    memcpy(&sz, v, 4);
                    value_ = new char[sz + 5];
                    memcpy(value_, v, sz + 5);
                    break;
                case Bson::k_int32:
                    value_ = new char[4];
                    memcpy(value_, v, 4);
                    break;
                case Bson::k_double:
                case Bson::k_int64:
                case Bson::k_timestamp:
                    value_ = new char[8];
                    memcpy(value_, v, 8);
                    break;
                case Bson::k_boolean:
                    value_ = new char[1];
                    memcpy(value_, v, 1);
                    break;
                case Bson::k_null:
                    value_ = NULL;
                    break;
                case Bson::k_document:
                case Bson::k_array:
                    last_child_ = 0;
                    subdocument(type_, *this, v);
                    break;
                case Bson::k_binary_document:
                    memcpy(&sz, v, 4);
                    value_ = new char[sz];
                    memcpy(value_, v, sz);
                    break;
                default:
                    break;
            }
        }
        
        // Clean up any old memory.
        if (old)
        {
            delete[] old;
        }
    }
    
    void Bson::nullify()
    {
        set_value(Bson::k_null, NULL);
    }

    void Bson::destroy()
    {
        set_value(Bson::k_document, NULL);
    }
    
    Bson& Bson::copy_from(const Bson& o)
    {
        destroy();
        type_ = o.type();
        if (k_document == o.type())
        {
            for (Linked_map<std::string, Bson *>::const_iterator iter = o.child_map_.begin();
                 o.child_map_.end() != iter;
                 ++iter)
            {
                Bson *ptr = new Bson(*(iter->second));
                set_child(lj::bson_escape_path(iter->first), ptr);
            }
        }
        else if (k_array == o.type())
        {
            for (Linked_map<std::string, Bson *>::const_iterator iter = o.child_map_.begin();
                 o.child_map_.end() != iter;
                 ++iter)
            {
                Bson *ptr = new Bson(*(iter->second));
                push_child("", ptr);
            }
        }
        else
        {
            set_value(o.type_, o.value_);
        }
        return *this;
    }
    
    namespace {
        void split_path(const std::string& path, std::list<std::string>& parts)
        {
            const char* tmp = path.c_str();
            std::string current;
            for (;*tmp; ++tmp)
            {
                if (*tmp == '/')
                {
                    if (current.size() > 0)
                    {
                        parts.push_back(current);
                        current.erase();
                    }
                }
                else if (*tmp == '\\' && *(tmp + 1))
                {
                    current.push_back(*(++tmp));
                }
                else
                {
                    current.push_back(*tmp);
                }
            }
            if (current.size() > 0)
            {
                parts.push_back(current);
            }
        }
    };
    
    Bson* Bson::path(const std::string& p)
    {
        std::list<std::string> parts;
        split_path(p, parts);
        Bson *n = this;
        while (0 < parts.size())
        {
            Linked_map<std::string, Bson*>::iterator iter = n->child_map_.find(parts.front());
            if (n->child_map_.end() == iter)
            {
                iter = n->child_map_.insert(std::pair<std::string, Bson*>(parts.front(), new Bson())).first;
            }
            n = iter->second;
            parts.pop_front();
        }
        return n;
    }
    
    const Bson* Bson::path(const std::string& p) const
    {
        std::list<std::string> parts;
        split_path(p, parts);
        const Bson *n = this;
        while (0 < parts.size())
        {
            Linked_map<std::string, Bson*>::const_iterator iter = n->child_map_.find(parts.front());
            if (n->child_map_.end() == iter)
            {
                return 0;
            }
            n = iter->second;
            parts.pop_front();
        }
        return n;
    }
    
    void Bson::set_child(const std::string& p, Bson* c)
    {
        std::list<std::string> parts;
        split_path(p, parts);

        if (!parts.size()) 
        {
            // No child name or path, so abort.
            return;
        }
        std::string child_name = parts.back();
        parts.pop_back();
        
        // navigate the structure.
        Bson *n = this;
        while (parts.size())
        {
            Linked_map<std::string, Bson*>::iterator iter = n->child_map_.find(parts.front());
            if (n->child_map_.end() == iter)
            {
                iter = n->child_map_.insert(std::pair<std::string, Bson*>(parts.front(), new Bson())).first;
            }
            n = iter->second;
            parts.pop_front();
        }
        
        // remove any existing value just in case.
        Linked_map<std::string, Bson*>::iterator iter = n->child_map_.find(child_name);
        if (n->child_map_.end() != iter)
        {
            if(iter->second == c)
            {
                //already here, do nothing.
                return;
            }
            delete iter->second;
            n->child_map_.erase(iter);
        }
        
        if (c)
        {
            if(Bson::k_document != n->type())
            {
                // If this isn't a document, convert it to a document.
                n->set_value(Bson::k_document, NULL);
            }
            
            // Add the child.
            n->child_map_.insert(std::pair<std::string, Bson*>(child_name, c));
        }
        else
        {
            // Remove the child.
            n->child_map_.erase(child_name);
        }
    }
    
    void Bson::push_child(const std::string& p, Bson* c)
    {
        if (!c)
        {
            // Cannot push null.
            return;
        }
        
        Bson* ptr = path(p);
        if(Bson::k_array != ptr->type())
        {
            // If this isn't an array, convert it to an array.
            ptr->set_value(Bson::k_array, NULL);
        }
        
        //std::ostringstream oss;
        //oss << ptr->last_child_++;
        //ptr->child_map_.insert(std::pair<std::string, Bson*>(oss.str(), c));
        char key[20];
        sprintf(key, "%d", last_child_++);
        std::string tmp(key);
        ptr->child_map_.insert(std::pair<std::string, Bson*>(tmp, c));
    }
    
    Bson& Bson::operator<<(const Bson& o)
    {
        if(Bson::k_array != type())
        {
            // If this isn't an array, convert it to an array.
            set_value(Bson::k_array, NULL);
        }
        
        std::ostringstream oss;
        oss << last_child_++;
        child_map_.insert(std::pair<std::string, Bson*>(oss.str(), new Bson(o)));
        return *this;
    }
        
    bool Bson::exists() const
    {
        if (bson_type_is_nested(type()))
        {
            return (0 < child_map_.size() ? true : false);
        }
        else
        {
            return (value_ ? true : (type() == Bson::k_null ? true : false));
        }
    }
    
    size_t Bson::size() const
    {
        long sz = 0;
        switch (type())
        {
            case Bson::k_string:
                memcpy(&sz, value_, 4);
                sz += 4;
                break;
            case Bson::k_binary:
                memcpy(&sz, value_, 4);
                sz += 5;
                break;
            case Bson::k_int32:
                sz = 4;
                break;
            case Bson::k_double:
            case Bson::k_int64:
            case Bson::k_timestamp:
                sz = 8;
                break;
            case Bson::k_boolean:
                sz = 1;
                break;
            case Bson::k_null:
                sz = 0;
                break;
            case Bson::k_document:
            case Bson::k_array:
                sz += 5;
                for (Linked_map<std::string, Bson *>::const_iterator iter = to_map().begin();
                     to_map().end() != iter;
                     ++iter)
                {
                    sz += iter->second->size() + iter->first.size() + 2;
                }
                break;
            case Bson::k_binary_document:
                memcpy(&sz, value_, 4);
                break;
            default:
                break;
        }
        return sz;
    }    
        
    size_t Bson::copy_to_bson(char* ptr) const
    {
        size_t sz = size();
        switch (type())
        {
            case Bson::k_document:
            case Bson::k_array:
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for (Linked_map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                     child_map_.end() != iter;
                     ++iter)
                {
                    Bson::Type t = iter->second->type();
                    if (Bson::k_binary_document == t)
                    {
                        t = Bson::k_document;
                    }
                    memcpy(ptr++, &t, 1);
                    memcpy(ptr, iter->first.c_str(), iter->first.size() + 1);
                    ptr += iter->first.size() + 1;
                    ptr += iter->second->copy_to_bson(ptr);
                }
                *ptr = 0;
                break;
            default:
                memcpy(ptr, value_, sz);
                break;
        }
        return sz;
    }
    
    std::string bson_escape_path(const std::string& input)
    {
        std::string name;
        for (std::string::const_iterator iter = input.begin();
             input.end() != iter;
             ++iter)
        {
            if ('/' == *iter)
            {
                name.push_back('\\');
            }
            name.push_back(*iter);
        }
        return name;
    }
    
    Bson* bson_new_string(const std::string& str)
    {
        long sz = str.size() + 1;
        char *ptr = new char[sz + 4];;
        memcpy(ptr, &sz, 4);
        memcpy(ptr + 4, str.c_str(), sz);
        Bson* new_bson = new Bson(Bson::k_string, ptr);
        delete[] ptr;
        return new_bson;
    }
    
    Bson* bson_new_boolean(const bool val)
    {
        return new Bson(Bson::k_boolean, reinterpret_cast<const char *> (&val));
    }
    
    Bson* bson_new_int64(const int64_t val)
    {
        return new Bson(Bson::k_int64, reinterpret_cast<const char *> (&val));
    }
    
    Bson* bson_new_uint64(const uint64_t val)
    {
        return new Bson(Bson::k_int64, reinterpret_cast<const char*> (&val));
    }
    
    Bson* bson_new_null()
    {
        return new Bson(Bson::k_null, NULL);
    }

    Bson* bson_new_binary(const char* val, uint32_t sz, Bson::Binary_type subtype)
    {
        char* ptr = new char[sz + 5];
        memcpy(ptr, &sz, 4);
        memcpy(ptr + 4, &subtype, 1);
        memcpy(ptr + 5, val, sz);
        Bson* new_bson = new Bson(Bson::k_binary, ptr);
        delete[] ptr;
        return new_bson;
    }

    lj::Bson* bson_new_cost(const std::string& cmd,
                            unsigned long long time,
                            long long filter_size,
                            long long result_size)
    {
        lj::Bson* b = new lj::Bson();
        b->set_child("cmd", lj::bson_new_string(cmd));
        b->set_child("usecs", lj::bson_new_uint64(time));
        b->set_child("filter_size", lj::bson_new_int64(filter_size));
        b->set_child("result_size", lj::bson_new_int64(result_size));
        return b;
    }
    
    std::string bson_as_debug_string(const Bson& b)
    {
        Bson::Binary_type binary_type = Bson::k_bin_user_defined;
        long long l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        if (!bson_type_is_nested(b.type()) && !v)
        {
            return std::string();
        }
        
        std::ostringstream buf;
        switch (b.type())
        {
            case Bson::k_string:
                memcpy(&l, v, 4);
                buf << "(4-" << l << ")" << (v + 4);
                return buf.str();
            case Bson::k_binary:
                memcpy(&l, v, 4);
                memcpy(&binary_type, v, 1);
                buf << "(4-" << l << ")";
                buf << "(1-" << bson_binary_type_string(binary_type) << ")";
                buf << lj::base64_encode(reinterpret_cast<const unsigned char*>(v + 5), l);
                return buf.str();
            case Bson::k_int32:
                memcpy(&l, v, 4);
                buf << "(4)" << l;
                return buf.str();
            case Bson::k_double:
                memcpy(&d, v, 8);
                buf << "(8)" << d;
                return buf.str();
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                buf << "(8)" << l;
                return buf.str();
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                buf << "(1)" << ((bool)l);
                return buf.str();
            case Bson::k_document:
            case Bson::k_array:
                if (!b.to_map().size())
                {
                    return "{(4-0)(1-0)}";
                }
                buf << "{(4-" << b.size() << ")";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    buf << "(1-" << bson_type_string(iter->second->type()) << ")";
                    buf << "\"(" << iter->first.size() + 1 << ")" << escape(iter->first) << "\":";
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << bson_as_debug_string(*iter->second);
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("(1-0)}");
            case Bson::k_binary_document:
                buf << bson_as_debug_string(Bson(Bson::k_document, b.to_value()));
                break;
            default:
                break;
        }
        return std::string();
    }
    
    std::string bson_as_string(const Bson& b)
    {
        long long l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        if (!bson_type_is_nested(b.type()) && !v)
        {
            return std::string();
        }
        
        std::ostringstream buf;
        switch (b.type())
        {
            case Bson::k_string:
                memcpy(&l, v, 4);
                return std::string(v + 4);
            case Bson::k_binary:
                memcpy(&l, v, 4);
                return lj::base64_encode(reinterpret_cast<const unsigned char*>(v + 5), l);
            case Bson::k_int32:
                memcpy(&l, v, 4);
                buf << l;
                return buf.str();
            case Bson::k_double:
                memcpy(&d, v, 8);
                buf << d;
                return buf.str();
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                buf << l;
                return buf.str();
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                buf << ((bool)l);
                return buf.str();
            case Bson::k_null:
                return std::string("null");
            case Bson::k_document:
                if (!b.to_map().size())
                {
                    return "{}";
                }
                buf << "{";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    buf << "\"" << escape(iter->first) << "\":";
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << bson_as_string(*iter->second);
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("}");
            case Bson::k_array:
                if (!b.to_map().size())
                {
                    return "[]";
                }
                buf << "[";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << bson_as_string(*iter->second);
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("]");
            case Bson::k_binary_document:
                buf << bson_as_string(Bson(Bson::k_document, b.to_value()));
                break;
            default:
                break;
        }
        return std::string();
    }
    
    std::string bson_as_pretty_string(const Bson& b, int lvl)
    {
        long long l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        if (!bson_type_is_nested(b.type()) && !v)
        {
            return std::string();
        }
        
        std::ostringstream buf;
        std::string indent;
        for (int h = 0; h < lvl; ++h)
        {
            indent.append("  ");
        }                
        
        switch (b.type())
        {
            case Bson::k_string:
                memcpy(&l, v, 4);
                return std::string(v + 4);
            case Bson::k_binary:
                memcpy(&l, v, 4);
                return lj::base64_encode(reinterpret_cast<const unsigned char*>(v + 5), l);
            case Bson::k_int32:
                memcpy(&l, v, 4);
                buf << l;
                return buf.str();
            case Bson::k_double:
                memcpy(&d, v, 8);
                buf << d;
                return buf.str();
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                buf << l;
                return buf.str();
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                buf << ((bool)l);
                return buf.str();
            case Bson::k_null:
                return std::string("null");
            case Bson::k_document:
                if (!b.to_map().size())
                {
                    return "{}";
                }
                buf << "{\n";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    if (!iter->second->exists())
                    {
                        continue;
                    }
                    buf << indent << "  \"" << escape(iter->first) << "\":";
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << bson_as_pretty_string(*iter->second, lvl + 1);
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",\n";
                }
                return buf.str().erase(buf.str().size() - 2).append("\n").append(indent).append("}");
            case Bson::k_array:
                if (!b.to_map().size())
                {
                    return "[]";
                }
                buf << "[ \n";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    if (!iter->second->exists())
                    {
                        continue;
                    }
                    buf << indent << "  ";
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << bson_as_pretty_string(*iter->second, lvl + 1);
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",\n";
                }
                return buf.str().erase(buf.str().size() - 2).append("\n").append(indent).append("]");
            case Bson::k_binary_document:
                buf << bson_as_pretty_string(Bson(Bson::k_document, b.to_value()));
                break;
            default:
                break;
        }
        return std::string();
    }
    
    std::set<std::string> bson_as_key_set(const Bson& b)
    {
        std::set<std::string> key_set;
        std::set<std::string>::iterator set_iter = key_set.begin();
        if (Bson::k_document == b.type())
        {
            for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                 b.to_map().end() != iter;
                 ++iter)
            {
                set_iter = key_set.insert(set_iter, iter->first);
            }
        }
        return key_set;
    }
    
    std::set<std::string> bson_as_value_string_set(const Bson& b)
    {
        std::set<std::string> value_set;
        if (bson_type_is_nested(b.type()))
        {
            for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                 b.to_map().end() != iter;
                 ++iter)
            {
                value_set.insert(bson_as_string(*iter->second));
            }
        }
        else
        {
            value_set.insert(bson_as_string(b));
        }
        return value_set;
    }
    
    int32_t bson_as_int32(const Bson& b)
    {
        long l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        switch (b.type())
        {
            case Bson::k_string:
                return atoi(v + 4);
            case Bson::k_int32:
                memcpy(&l, v, 4);
                return (int)l;
            case Bson::k_double:
                memcpy(&d, v, 8);
                return (int)d;
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                return (int)l;
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                return (int)l;
            default:
                break;
        }
        return 0;
    }
    
    int64_t bson_as_int64(const Bson& b)
    {
        int64_t l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        switch (b.type())
        {
            case Bson::k_string:
                return atol(v + 4);
            case Bson::k_int32:
                memcpy(&l, v, 4);
                return l;
            case Bson::k_double:
                memcpy(&d, v, 8);
                return (long long)d;
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                return l;
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                return l;
            default:
                break;
        }
        return 0;
    }
    
    uint64_t bson_as_uint64(const Bson& b)
    {
        uint64_t l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        switch (b.type())
        {
            case Bson::k_string:
                return static_cast<uint64_t>(atol(v + 4));
            case Bson::k_int32:
                memcpy(&l, v, 4);
                return l;
            case Bson::k_double:
                memcpy(&d, v, 8);
                return (long long)d;
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                return l;
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                return l;
            default:
                break;
        }
        return 0;
    }
    
    bool bson_as_boolean(const Bson& b)
    {
        long l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        const char* s = v + 4;
        switch (b.type())
        {
            case Bson::k_string:
                if(!v) return false;
                if(!s[0]) return false;
                if(s[0] == '0' && !s[1]) return false;
                if(s[0] == '1' && !s[1]) return true;
                if(strlen(s) == 4 &&
                   toupper(s[0]) == 'T' &&
                   toupper(s[1]) == 'R' &&
                   toupper(s[2]) == 'U' &&
                   toupper(s[3]) == 'E')
                    return true;
            case Bson::k_int32:
                memcpy(&l, v, 4);
                return l;
            case Bson::k_double:
                memcpy(&d, v, 8);
                return (long)d;
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                return l;
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                return l;
            default:
                break;
        }
        return false;
    }
    
    double bson_as_double(const Bson& b)
    {
        long l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        switch (b.type())
        {
            case Bson::k_string:
                return atof(v + 4);
            case Bson::k_int32:
                memcpy(&l, v, 4);
                return (double)l;
            case Bson::k_double:
                memcpy(&d, v, 8);
                return d;
            case Bson::k_int64:
            case Bson::k_timestamp:
                memcpy(&l, v, 8);
                return (double)l;
            case Bson::k_boolean:
                memcpy(&l, v, 1);
                return (double)l;
            default:
                break;
        }
        return 0.0;
    }
    
    const char* bson_as_binary(const Bson& b, Bson::Binary_type* t, uint32_t* sz)
    {
        if (Bson::k_binary != b.type())
        {
            return 0;
        }
        const char* v = b.to_value();
        memcpy(sz, v, 4);
        memcpy(t, v + 4, 1);
        return v + 5;
    }
    
    void bson_increment(lj::Bson& b, int amount)
    {
        int64_t v = lj::bson_as_int64(b) + amount;
        b.set_value(lj::Bson::k_int64,
                    reinterpret_cast<const char*> (&v));
    }
    
    void bson_save(const Bson& b, const std::string& path)
    {
        std::ofstream f(path.c_str());
        char *ptr = b.to_binary();
        f.write(ptr, b.size());
        f.close();
        delete[] ptr;
    }
    
    Bson* bson_load(const std::string& path)
    {
        std::ifstream f(path.c_str());
        size_t sz = 0;
        f.read((char *)&sz, 4);
        char *ptr = new char[sz];
        memcpy(ptr, &sz, 4);
        f.read(ptr + 4, sz - 4);
        Bson* doc = new Bson(Bson::k_document, ptr);
        f.close();
        delete[] ptr;
        return doc;
    }
}; // namespace lj
