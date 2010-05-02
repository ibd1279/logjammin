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
                node.child(name, Bson(static_cast<Bson_node_type>(t), ptr));
                sz = 0;
                switch(static_cast<Bson_node_type>(t)) {
                    case k_bson_string:
                        memcpy(&sz, ptr, 4);
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
                    case k_bson_document:
                    case k_bson_array:
                        memcpy(&sz, ptr, 4);
                        break;
                    default:
                        break;
                }
                ptr += sz;
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
    
    Bson::Bson() : child_map_(), child_array_(), value_(0), type_(k_bson_document)
    {
    }
    
    Bson::Bson(const Bson_node_type t, const char *v) : child_map_(), child_array_(), value_(0), type_(k_bson_document)
    {
        set_value(t, v);
    }
    
    Bson::Bson(const Bson &o) : child_map_(), child_array_(), value_(0), type_(o.type_)
    {
        assign(o);
    }
    
    Bson::~Bson()
    {
        if (value_)
        {
            delete[] value_;
        }
        for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
             child_map_.end() != iter;
             ++iter)
        {
            delete iter->second;
        }
    }
    
    //=====================================================================
    // Bson Instance
    //=====================================================================
    
    void Bson::set_value(const Bson_node_type t, const char* v)
    {
        // assume the type may have changed.
        char* old = NULL;
        if (bson_type_is_nested(type()))
        {
            for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                 child_map_.end() != iter;
                 ++iter)
            {
                delete iter->second;
            }
            child_map_.clear();
            child_array_.clear();
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
    
    void Bson::set_string(const std::string &v)
    {
        long sz = v.size() + 1;
        char *ptr = new char[sz + 4];;
        memcpy(ptr, &sz, 4);
        memcpy(ptr + 4, v.c_str(), sz);
        set_value(k_bson_string, ptr);
        delete[] ptr;
    }
    
    void Bson::set_int32(int32_t v)
    {
        char *ptr = reinterpret_cast<char *>(&v);
        set_value(k_bson_int32, ptr);
    }
    
    void Bson::set_int64(int64_t v)
    {
        char *ptr = reinterpret_cast<char *>(&v);
        set_value(k_bson_int64, ptr);
    }
    
    void Bson::set_double(double v)
    {
        char *ptr = reinterpret_cast<char *>(&v);
        set_value(k_bson_double, ptr);
    }
    void Bson::set_boolean(bool v)
    {
        char *ptr = reinterpret_cast<char *>(&v);
        set_value(k_bson_boolean, ptr);
    }
    void Bson::nullify()
    {
        set_value(k_bson_null, NULL);
    }

    void Bson::destroy()
    {
        set_value(k_bson_document, NULL);
    }
    
    Bson& Bson::assign(const Bson& o)
    {
        destroy();
        if (bson_type_is_nested(o.type()))
        {
            for (std::map<std::string, Bson *>::const_iterator iter = o.child_map_.begin();
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
    
    Bson& Bson::child(const std::string& n, const Bson& c)
    {
        std::map<std::string, Bson*>::iterator iter = child_map_.find(n);
        Bson* ptr = new Bson(c);
        if (child_map_.end() != iter)
        {
            delete iter->second;
            child_map_.erase(iter);
        }
        child_map_.insert(std::pair<std::string, Bson *>(n, ptr));
        return *ptr;
    }
    
    std::string Bson::to_dbg_s() const
    {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch (type())
        {
            case k_bson_string:
                memcpy(&l, value_, 4);
                buf << "(4-" << l << ")" << "(" << l << ")" << (value_ + 4);
                return buf.str();
            case k_bson_int32:
                memcpy(&l, value_, 4);
                buf << "(4)" << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, value_, 8);
                buf << "(8)" << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, value_, 8);
                buf << "(8)" << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, value_, 1);
                buf << "(1)" << ((bool)l);
                return buf.str();
            case k_bson_document:
            case k_bson_array:
                if (!child_map_.size())
                {
                    return "{(4-0)(1-0)}";
                }
                buf << "{(4-" << size() << ")";
                for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                     child_map_.end() != iter;
                     ++iter)
                {
                    buf << "(1-" << bson_type_string(iter->second->type()) << ")";
                    buf << "\"(" << iter->first.size() + 1 << ")" << escape(iter->first) << "\":";
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << iter->second->to_s();
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

    std::string Bson::to_s() const
    {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch (type())
        {
            case k_bson_string:
                memcpy(&l, value_, 4);
                return std::string(value_ + 4);
            case k_bson_int32:
                memcpy(&l, value_, 4);
                buf << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, value_, 8);
                buf << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, value_, 8);
                buf << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, value_, 1);
                buf << ((bool)l);
                return buf.str();
            case k_bson_null:
                return std::string("null");
            case k_bson_document:
            case k_bson_array:
                if (!child_map_.size())
                {
                    return "{}";
                }
                buf << "{";
                for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                     child_map_.end() != iter;
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
                    buf << iter->second->to_s();
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("}");
            default:
                break;
        }
        return std::string();
    }
    
    std::string Bson::to_pretty_s(int lvl) const
    {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        std::string indent;
        switch (type())
        {
            case k_bson_string:
                memcpy(&l, value_, 4);
                return std::string(value_ + 4);
            case k_bson_int32:
                memcpy(&l, value_, 4);
                buf << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, value_, 8);
                buf << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, value_, 8);
                buf << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, value_, 1);
                buf << ((bool)l);
                return buf.str();
            case k_bson_null:
                return std::string("null");
            case k_bson_document:
            case k_bson_array:
                if (!child_map_.size())
                {
                    return "{}";
                }
                buf << "{\n";
                for (int h = 0; h < lvl; ++h)
                {
                    indent.append("  ");
                }
                for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                     child_map_.end() != iter;
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
                    buf << iter->second->to_pretty_s(lvl + 1);
                    if (bson_type_is_quotable(iter->second->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",\n";
                }
                return buf.str().erase(buf.str().size() - 2).append("\n").append(indent).append("}");
            default:
                break;
        }
        return std::string();
    }
    
    std::set<std::string> Bson::to_set() const
    {
        std::set<std::string> f;
        switch (type())
        {
            case k_bson_document:
            case k_bson_array:
                for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                     child_map_.end() != iter;
                     ++iter)
                {
                    if(iter->second->exists()) 
                    {
                        f.insert(iter->second->to_s());
                    }
                }
                break;
            default:
                if (exists())
                {
                    f.insert(to_s());
                }
                break;
        }
        return f;
    }
    
    std::list<std::string> Bson::to_list() const
    {
        std::list<std::string> f;
        switch (type())
        {
            case k_bson_document:
            case k_bson_array:
                for(std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                    child_map_.end() != iter;
                    ++iter)
                {
                    if(iter->second->exists())
                    {
                        f.push_back(iter->second->to_s());
                    }
                }
                break;
            default:
                if (exists())
                {
                    f.push_back(to_s());
                }
                break;
        }
        return f;
    }
    
    int Bson::to_i() const
    {
        long l = 0;
        double d = 0.0;
        switch (type())
        {
            case k_bson_string:
                return atoi(value_ + 4);
            case k_bson_int32:
                memcpy(&l, value_, 4);
                return (int)l;
            case k_bson_double:
                memcpy(&d, value_, 8);
                return (int)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, value_, 8);
                return (int)l;
            case k_bson_boolean:
                memcpy(&l, value_, 1);
                return (int)l;
            default:
                break;
        }
        return 0;
    }
    
    long long Bson::to_l() const
    {
        long l = 0;
        double d = 0.0;
        switch (type())
        {
            case k_bson_string:
                return atol(value_ + 4);
            case k_bson_int32:
                memcpy(&l, value_, 4);
                return l;
            case k_bson_double:
                memcpy(&d, value_, 8);
                return (long long)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, value_, 8);
                return l;
            case k_bson_boolean:
                memcpy(&l, value_, 1);
                return l;
            default:
                break;
        }
        return 0;
    }
    
    bool Bson::to_b() const
    {
        long l = 0;
        double d = 0.0;
        char* s = value_ + 4;
        switch (type())
        {
            case k_bson_string:
                if(!value_) return false;
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
                memcpy(&l, value_, 4);
                return l;
            case k_bson_double:
                memcpy(&d, value_, 8);
                return (long)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, value_, 8);
                return l;
            case k_bson_boolean:
                memcpy(&l, value_, 1);
                return l;
            default:
                break;
        }
        return false;
    }
    
    double Bson::to_d() const
    {
        long l = 0;
        double d = 0.0;
        switch (type())
        {
            case k_bson_string:
                return atof(value_ + 4);
            case k_bson_int32:
                memcpy(&l, value_, 4);
                return (double)l;
            case k_bson_double:
                memcpy(&d, value_, 8);
                return d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, value_, 8);
                return (double)l;
            case k_bson_boolean:
                memcpy(&l, value_, 1);
                return (double)l;
            default:
                break;
        }
        return 0.0;
    }
    
    char* Bson::bson() const
    {
        char *ptr = new char[size()];
        copy_to_bson(ptr);
        return ptr;
    }
    
    size_t Bson::copy_to_bson(char* ptr) const
    {
        size_t sz = size();
        switch (type())
        {
            case k_bson_document:
            case k_bson_array:
                sz = size();
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
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
    
    std::set<std::string> Bson::children() const
    {
        std::set<std::string> f;
        if (bson_type_is_nested(type()))
        {
            for (std::map<std::string, Bson*>::const_iterator iter = child_map_.begin();
                 child_map_.end() != iter;
                 ++iter)
            {
                if (iter->second->exists())
                {
                    f.insert(iter->first);
                }
            }
        }
        return f;
    }
    
    Bson &Bson::child(const std::string& n)
    {
        std::map<std::string, Bson*>::iterator iter = child_map_.find(n);
        if (child_map_.end() != iter)
        {
            return *(iter->second);
        }
        Bson *ptr = new Bson();
        child_map_.insert(std::pair<std::string, Bson *>(n, ptr));
        return *ptr;
    }
    
    const Bson &Bson::child(const std::string& n) const
    {
        std::map<std::string, Bson*>::const_iterator iter = child_map_.find(n);
        if (child_map_.end() == iter)
        {
            throw new Exception("DocumentError", std::string("Unable to find child [").append(n).append("]."));
        }
        return *(iter->second);
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
        const Bson& navigate_document(const Bson& n, std::list<std::string>& p)
        {
            if (p.size() < 1)
            {
                return n;
            }
            std::string front = p.front();
            p.pop_front();
            return navigate_document(n.child(front), p);
        }
        Bson& navigate_document(Bson& n, std::list<std::string>& p)
        {
            if (p.size() < 1)
            {
                return n;
            }
            std::string front = p.front();
            p.pop_front();
            return navigate_document(n.child(front), p);
        }
    };
    
    Bson& Bson::nav(const std::string& p)
    {
        std::list<std::string> parts;
        split_path(p, parts);
        return navigate_document(*this, parts);
    }
    
    const Bson& Bson::nav(const std::string& p) const
    {
        std::list<std::string> parts;
        split_path(p, parts);
        return navigate_document(*this, parts);
    }
    
    bool Bson::exists() const
    {
        return child_map_.size() ? true : (value_ ? true : false);
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
                for (std::map<std::string, Bson *>::const_iterator iter = child_map_.begin();
                     child_map_.end() != iter;
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
    
    const Bson& Bson::save(const std::string& fn) const
    {
        std::ofstream f(fn.c_str());
        char *ptr = bson();
        f.write(ptr, size());
        f.close();
        delete[] ptr;
        return *this;
    }
    
    Bson& Bson::load(const std::string& fn)
    {
        std::ifstream f(fn.c_str());
        size_t sz = 0;
        f.read((char *)&sz, 4);
        char *ptr = new char[sz];
        memcpy(ptr, &sz, 4);
        f.read(ptr + 4, sz - 4);
        set_value(k_bson_document, ptr);
        f.close();
        delete[] ptr;
        return *this;
    }
};