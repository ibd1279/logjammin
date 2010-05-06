/*!
 \file Bson.cpp
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

#include "build/default/config.h"
#include "Bson.h"
#include "Exception.h"
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>

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
        
        void subdocument(Bson &node, const char *value) {
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
                Bson* new_child = new Bson(static_cast<Bson_node_type>(t), ptr);
                node.set_child(name, new_child);
                ptr += new_child->size();
            }
        }
        
        std::string k_bson_type_string_string("string");
        std::string k_bson_type_string_int32("int32");
        std::string k_bson_type_string_double("double");
        std::string k_bson_type_string_int64("int64");
        std::string k_bson_type_string_timestamp("timestamp");
        std::string k_bson_type_string_boolean("boolean");
        std::string k_bson_type_string_null("null");
        std::string k_bson_type_string_document("document");
        std::string k_bson_type_string_array("array");
        std::string k_bson_type_string_unknown("unknown");
    };
    
    const std::string& bson_type_string(const Bson_node_type t)
    {
        switch (t)
        {
            case k_bson_string:
                return k_bson_type_string_string;
            case k_bson_int32:
                return k_bson_type_string_int32;
            case k_bson_double:
                return k_bson_type_string_double;
            case k_bson_int64:
                return k_bson_type_string_int64;
            case k_bson_timestamp:
                return k_bson_type_string_timestamp;
            case k_bson_boolean:
                return k_bson_type_string_boolean;
            case k_bson_null:
                return k_bson_type_string_null;
            case k_bson_document:
                return k_bson_type_string_document;
            case k_bson_array:
                return k_bson_type_string_array;
            default:
                break;
        }
        return k_bson_type_string_unknown;
    }
    
    size_t bson_type_min_size(const Bson_node_type t)
    {
        switch (t)
        {
            case k_bson_string:
                return 5;
            case k_bson_int32:
                return 4;
            case k_bson_timestamp:
            case k_bson_int64:
            case k_bson_double:
                return 8;
            case k_bson_boolean:
                return 1;
            case k_bson_null:
                return 0;
            case k_bson_document:
            case k_bson_array:
                return 5;
            default:
                break;
        }
        return 5;
    }
    
    //=====================================================================
    // Bson ctor/dtor
    //=====================================================================
    
    Bson::Bson() : child_map_(), last_child_(0), value_(0), type_(k_bson_document)
    {
    }
    
    Bson::Bson(const Bson_node_type t, const char *v) : child_map_(), last_child_(0), value_(0), type_(k_bson_document)
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
    
    void Bson::set_value(const Bson_node_type t, const char* v)
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
                case k_bson_string:
                    memcpy(&sz, v, 4);
                    value_ = new char[sz + 4];
                    memcpy(value_, v, sz + 4);
                    break;
                case k_bson_int32:
                    value_ = new char[4];
                    memcpy(value_, v, 4);
                    break;
                case k_bson_double:
                case k_bson_int64:
                case k_bson_timestamp:
                    value_ = new char[8];
                    memcpy(value_, v, 8);
                    break;
                case k_bson_boolean:
                    value_ = new char[1];
                    memcpy(value_, v, 1);
                    break;
                case k_bson_null:
                    value_ = NULL;
                    break;
                case k_bson_document:
                    memcpy(&sz, v, 4);
                    subdocument(*this, v);
                    break;
                case k_bson_array:
                    memcpy(&sz, v, 4);
                    subdocument(*this, v);
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
        set_value(k_bson_null, NULL);
    }

    void Bson::destroy()
    {
        set_value(k_bson_document, NULL);
    }
    
    Bson& Bson::copy_from(const Bson& o)
    {
        destroy();
        if (bson_type_is_nested(o.type()))
        {
            for (Linked_map<std::string, Bson *>::const_iterator iter = o.child_map_.begin();
                 o.child_map_.end() != iter;
                 ++iter)
            {
                Bson *ptr = new Bson(*(iter->second));
                child_map_.insert(std::pair<std::string, Bson *>(iter->first, ptr));
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
        
        // Add the child.
        n->child_map_.insert(std::pair<std::string, Bson*>(child_name, c));
    }
    
    //! Push a child at a specific path.
    void Bson::push_child(const std::string& p, Bson* c)
    {
        std::ostringstream oss;
        oss << last_child_++;
        std::string tmp = oss.str();
        path(p)->child_map_.insert(std::pair<std::string, Bson*>(oss.str(), c));
    }
    
    //! Push a child onto this Bson object.
    Bson& Bson::operator<<(const Bson& o)
    {
        std::ostringstream oss;
        oss << last_child_;
        child_map_.insert(std::pair<std::string, Bson*>(oss.str(), new Bson(o)));
        return *this;
    }
        
    bool Bson::exists() const
    {
        if (bson_type_is_nested(type()))
        {
            return (0 < child_map_.size() ? child_map_.size() : 0);
        }
        else
        {
            return (value_ ? true : (type() == k_bson_null ? true : false));
        }
    }
    
    size_t Bson::size() const
    {
        long sz = 0;
        switch (type())
        {
            case k_bson_string:
                memcpy(&sz, value_, 4);
                sz += 4;
                break;
            case k_bson_int32:
                sz = 4;
                break;
            case k_bson_double:
            case k_bson_int64:
            case k_bson_timestamp:
                sz = 8;
                break;
            case k_bson_boolean:
                sz = 1;
                break;
            case k_bson_null:
                sz = 0;
                break;
            case k_bson_document:
            case k_bson_array:
                sz += 5;
                for (Linked_map<std::string, Bson *>::const_iterator iter = to_map().begin();
                     to_map().end() != iter;
                     ++iter)
                {
                    sz += iter->second->size() + iter->first.size() + 2;
                }
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
            case k_bson_document:
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for (Linked_map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                     child_map_.end() != iter;
                     ++iter)
                {
                    Bson_node_type t = iter->second->type();
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
    
    Bson* bson_new_string(const std::string& str)
    {
        long sz = str.size() + 1;
        char *ptr = new char[sz + 4];;
        memcpy(ptr, &sz, 4);
        memcpy(ptr + 4, str.c_str(), sz);
        Bson* new_bson = new Bson(k_bson_string, ptr);
        delete[] ptr;
        return new_bson;
    }
    
    Bson* bson_new_boolean(const bool val)
    {
        return new Bson(k_bson_boolean, reinterpret_cast<const char *> (&val));
    }
    
    Bson* bson_new_int64(const int64_t val)
    {
        return new Bson(k_bson_int64, reinterpret_cast<const char *> (&val));
    }
    
    std::string bson_as_debug_string(const Bson& b)
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
            case k_bson_string:
                memcpy(&l, v, 4);
                buf << "(4-" << l << ")" << "(" << l << ")" << (v + 4);
                return buf.str();
            case k_bson_int32:
                memcpy(&l, v, 4);
                buf << "(4)" << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, v, 8);
                buf << "(8)" << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, v, 8);
                buf << "(8)" << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, v, 1);
                buf << "(1)" << ((bool)l);
                return buf.str();
            case k_bson_document:
            case k_bson_array:
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
            case k_bson_string:
                memcpy(&l, v, 4);
                return std::string(v + 4);
            case k_bson_int32:
                memcpy(&l, v, 4);
                buf << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, v, 8);
                buf << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, v, 8);
                buf << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, v, 1);
                buf << ((bool)l);
                return buf.str();
            case k_bson_null:
                return std::string("null");
            case k_bson_document:
                if (!b.to_map().size())
                {
                    return "{}";
                }
                buf << "{";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    if (!iter->second->exists())
                    {
                        continue;
                    }
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
            case k_bson_array:
                if (!b.to_map().size())
                {
                    return "[]";
                }
                buf << "[";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    if (!iter->second->exists())
                    {
                        continue;
                    }
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
            case k_bson_string:
                memcpy(&l, v, 4);
                return std::string(v + 4);
            case k_bson_int32:
                memcpy(&l, v, 4);
                buf << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, v, 8);
                buf << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, v, 8);
                buf << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, v, 1);
                buf << ((bool)l);
                return buf.str();
            case k_bson_null:
                return std::string("null");
            case k_bson_document:
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
            case k_bson_array:
                if (!b.to_map().size())
                {
                    return "[]";
                }
                buf << "[";
                for (Linked_map<std::string, Bson*>::const_iterator iter = b.to_map().begin();
                     b.to_map().end() != iter;
                     ++iter)
                {
                    if (!iter->second->exists())
                    {
                        continue;
                    }
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << indent << "  " << bson_as_pretty_string(*iter->second, lvl + 1);
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",\n";
                }
                return buf.str().erase(buf.str().size() - 2).append("\n").append(indent).append("]");
            default:
                break;
        }
        return std::string();
    }
    
    std::set<std::string> bson_as_key_set(const Bson& b)
    {
        std::set<std::string> key_set;
        std::set<std::string>::iterator set_iter = key_set.begin();
        if (k_bson_document == b.type())
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
            case k_bson_string:
                return atoi(v + 4);
            case k_bson_int32:
                memcpy(&l, v, 4);
                return (int)l;
            case k_bson_double:
                memcpy(&d, v, 8);
                return (int)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, v, 8);
                return (int)l;
            case k_bson_boolean:
                memcpy(&l, v, 1);
                return (int)l;
            default:
                break;
        }
        return 0;
    }
    
    int64_t bson_as_int64(const Bson& b)
    {
        long l = 0;
        double d = 0.0;
        const char* v = b.to_value();
        switch (b.type())
        {
            case k_bson_string:
                return atol(v + 4);
            case k_bson_int32:
                memcpy(&l, v, 4);
                return l;
            case k_bson_double:
                memcpy(&d, v, 8);
                return (long long)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, v, 8);
                return l;
            case k_bson_boolean:
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
            case k_bson_string:
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
            case k_bson_int32:
                memcpy(&l, v, 4);
                return l;
            case k_bson_double:
                memcpy(&d, v, 8);
                return (long)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, v, 8);
                return l;
            case k_bson_boolean:
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
            case k_bson_string:
                return atof(v + 4);
            case k_bson_int32:
                memcpy(&l, v, 4);
                return (double)l;
            case k_bson_double:
                memcpy(&d, v, 8);
                return d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, v, 8);
                return (double)l;
            case k_bson_boolean:
                memcpy(&l, v, 1);
                return (double)l;
            default:
                break;
        }
        return 0.0;
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
        Bson* doc = new Bson(k_bson_document, ptr);
        f.close();
        delete[] ptr;
        return doc;
    }
}; // namespace lj