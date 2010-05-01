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
#include "BSONNode.h"
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
    };
    
    //=====================================================================
    // Bson ctor/dtor
    //=====================================================================
    
    Bson::Bson() : _children(), _value(NULL), _type(k_bson_document) {
    }
    
    Bson::Bson(const Bson_node_type t, const char *v) : _children(), _value(NULL), _type(k_bson_document) {
        set_value(t, v);
    }
    
    Bson::Bson(const Bson &o) : _children(), _value(NULL), _type(o._type) {
        assign(o);
    }
    
    Bson::~Bson() {
        if(_value)
            delete[] _value;
        for(childmap_t::const_iterator iter = _children.begin();
            iter != _children.end();
            ++iter) {
            delete iter->second;
        }
    }
    
    //=====================================================================
    // Bson Instance
    //=====================================================================
    
    Bson &Bson::set_value(const Bson_node_type t, const char *v) {
        // assume the type may have changed.
        char *old = NULL;
        if(nested()) {
            for(childmap_t::const_iterator iter = _children.begin();
                iter != _children.end();
                ++iter) {
                delete iter->second;
            }
            _children.clear();
            _value = NULL;
        } else {
            old = _value;
            _value = NULL;
        }
        
        // process the void pointer provided based on the provided type.
        _type = t;
        if(v) {
            long long sz = 0;
            switch(_type) {
                case k_bson_string:
                    memcpy(&sz, v, 4);
                    _value = new char[sz + 4];
                    memcpy(_value, v, sz + 4);
                    break;
                case k_bson_int32:
                    _value = new char[4];
                    memcpy(_value, v, 4);
                    break;
                case k_bson_double:
                case k_bson_int64:
                case k_bson_timestamp:
                    _value = new char[8];
                    memcpy(_value, v, 8);
                    break;
                case k_bson_boolean:
                    _value = new char[1];
                    memcpy(_value, v, 1);
                    break;
                case k_bson_null:
                    _value = NULL;
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
        if(old)
            delete[] old;
        
        return *this;
    }
    
    Bson &Bson::value(const std::string &v) {
        long sz = v.size() + 1;
        char *ptr = new char[sz + 4];;
        memcpy(ptr, &sz, 4);
        memcpy(ptr + 4, v.c_str(), sz);
        set_value(k_bson_string, ptr);
        delete[] ptr;
        return *this;
    }
    
    Bson &Bson::value(const int v) {
        char ptr[4];
        memcpy(ptr, &v, 4);
        set_value(k_bson_int32, ptr);
        return *this;
    }
    
    Bson &Bson::value(const long long v) {
        char ptr[8];
        memcpy(ptr, &v, 8);
        set_value(k_bson_int64, ptr);
        return *this;
    }
    
    Bson &Bson::value(const double v) {
        char ptr[8];
        memcpy(ptr, &v, 8);
        set_value(k_bson_double, ptr);
        return *this;
    }
    Bson &Bson::value(const bool v) {
        char ptr[1];
        memcpy(ptr, &v, 1);
        set_value(k_bson_boolean, ptr);
        return *this;
    }
    Bson &Bson::nullify() {
        set_value(k_bson_null, NULL);
        return *this;
    }

    Bson &Bson::destroy() {
        set_value(k_bson_document, NULL);
        return *this;
    }
    
    Bson &Bson::assign(const Bson &o) {
        destroy();
        if(o.nested()) {
            for(std::map<std::string, Bson *>::const_iterator iter = o._children.begin();
                iter != o._children.end();
                ++iter) {
                Bson *ptr = new Bson(*(iter->second));
                _children.insert(std::pair<std::string, Bson *>(iter->first, ptr));
            }
        } else {
            set_value(o._type, o._value);
        }
        return *this;
    }
    
    Bson &Bson::child(const std::string &n, const Bson &c) {
        childmap_t::iterator iter = _children.find(n);
        Bson *ptr = new Bson(c);
        if(iter != _children.end()) {
            delete iter->second;
            _children.erase(iter);
        }
        
        _children.insert(std::pair<std::string, Bson *>(n, ptr));
        return *ptr;
    }
    
    std::string Bson::to_dbg_s() const {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch(type()) {
            case k_bson_string:
                memcpy(&l, _value, 4);
                buf << "(4-" << l << ")" << "(" << l << ")" << (_value + 4);
                return buf.str();
            case k_bson_int32:
                memcpy(&l, _value, 4);
                buf << "(4)" << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, _value, 8);
                buf << "(8)" << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, _value, 8);
                buf << "(8)" << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, _value, 1);
                buf << "(1)" << ((bool)l);
                return buf.str();
            case k_bson_document:
            case k_bson_array:
                if(!_children.size())
                    return "{(4-0)(1-0)}";
                buf << "{(4-" << size() << ")";
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    buf << "(1-" << iter->second->type_string() << ")";
                    buf << "\"(" << iter->first.size() + 1 << ")" << escape(iter->first) << "\":";
                    if(iter->second->quotable())
                        buf << "\"";
                    buf << iter->second->to_s();
                    if(iter->second->quotable())
                        buf << "\"";
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("(1-0)}");
            default:
                break;
        }
        return std::string();
    }

    std::string Bson::to_s() const {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch(type()) {
            case k_bson_string:
                memcpy(&l, _value, 4);
                return std::string(_value + 4);
            case k_bson_int32:
                memcpy(&l, _value, 4);
                buf << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, _value, 8);
                buf << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, _value, 8);
                buf << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, _value, 1);
                buf << ((bool)l);
                return buf.str();
            case k_bson_null:
                return std::string("null");
            case k_bson_document:
            case k_bson_array:
                if(!_children.size())
                    return "{}";
                buf << "{";
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    if(!iter->second->exists())
                        continue;
                    buf << "\"" << escape(iter->first) << "\":";
                    if(iter->second->type() == k_bson_string)
                        buf << "\"";
                    buf << iter->second->to_s();
                    if(iter->second->type() == k_bson_string)
                        buf << "\"";
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("}");
            default:
                break;
        }
        return std::string();
    }
    
    std::string Bson::to_pretty_s(int lvl) const {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        std::string indent;
        switch(type()) {
            case k_bson_string:
                memcpy(&l, _value, 4);
                return std::string(_value + 4);
            case k_bson_int32:
                memcpy(&l, _value, 4);
                buf << l;
                return buf.str();
            case k_bson_double:
                memcpy(&d, _value, 8);
                buf << d;
                return buf.str();
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, _value, 8);
                buf << l;
                return buf.str();
            case k_bson_boolean:
                memcpy(&l, _value, 1);
                buf << ((bool)l);
                return buf.str();
            case k_bson_null:
                return std::string("null");
            case k_bson_document:
            case k_bson_array:
                if(!_children.size())
                    return "{}";
                buf << "{\n";
                for(int h = 0; h < lvl; ++h)
                    indent.append("  ");
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    if(!iter->second->exists())
                        continue;
                    buf << indent << "  \"" << escape(iter->first) << "\":";
                    if(iter->second->type() == k_bson_string)
                        buf << "\"";
                    buf << iter->second->to_pretty_s(lvl + 1);
                    if(iter->second->type() == k_bson_string)
                        buf << "\"";
                    buf << ",\n";
                }
                return buf.str().erase(buf.str().size() - 2).append("\n").append(indent).append("}");
            default:
                break;
        }
        return std::string();
    }
    
    std::set<std::string> Bson::to_set() const {
        std::set<std::string> f;
        switch(type()) {
            case k_bson_document:
            case k_bson_array:
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    if(iter->second->exists()) 
                        f.insert(iter->second->to_s());
                }
                break;
            default:
                if(exists())
                    f.insert(to_s());
                break;
        }
        return f;
    }
    
    std::list<std::string> Bson::to_list() const {
        std::list<std::string> f;
        switch(type()) {
            case k_bson_document:
            case k_bson_array:
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    if(iter->second->exists())
                        f.push_back(iter->second->to_s());
                }
                break;
            default:
                if(exists())
                    f.push_back(to_s());
                break;
        }
        return f;
    }
    
    int Bson::to_i() const {
        long l = 0;
        double d = 0.0;
        switch(type()) {
            case k_bson_string:
                return atoi(_value + 4);
            case k_bson_int32:
                memcpy(&l, _value, 4);
                return (int)l;
            case k_bson_double:
                memcpy(&d, _value, 8);
                return (int)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, _value, 8);
                return (int)l;
            case k_bson_boolean:
                memcpy(&l, _value, 1);
                return (int)l;
            default:
                break;
        }
        return 0;
    }
    
    long long Bson::to_l() const {
        long l = 0;
        double d = 0.0;
        switch(type()) {
            case k_bson_string:
                return atol(_value + 4);
            case k_bson_int32:
                memcpy(&l, _value, 4);
                return l;
            case k_bson_double:
                memcpy(&d, _value, 8);
                return (long long)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, _value, 8);
                return l;
            case k_bson_boolean:
                memcpy(&l, _value, 1);
                return l;
            default:
                break;
        }
        return 0;
    }
    
    bool Bson::to_b() const  {
        long l = 0;
        double d = 0.0;
        char *s = _value + 4;
        switch(type()) {
            case k_bson_string:
                if(!_value) return false;
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
                memcpy(&l, _value, 4);
                return l;
            case k_bson_double:
                memcpy(&d, _value, 8);
                return (long)d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, _value, 8);
                return l;
            case k_bson_boolean:
                memcpy(&l, _value, 1);
                return l;
            default:
                break;
        }
        return false;
    }
    
    double Bson::to_d() const {
        long l = 0;
        double d = 0.0;
        switch(type()) {
            case k_bson_string:
                return atof(_value + 4);
            case k_bson_int32:
                memcpy(&l, _value, 4);
                return (double)l;
            case k_bson_double:
                memcpy(&d, _value, 8);
                return d;
            case k_bson_int64:
            case k_bson_timestamp:
                memcpy(&l, _value, 8);
                return (double)l;
            case k_bson_boolean:
                memcpy(&l, _value, 1);
                return (double)l;
            default:
                break;
        }
        return 0.0;
    }
    
    char *Bson::bson() const {
        char *ptr = new char[size()];
        copy_to_bson(ptr);
        return ptr;
    }
    
    size_t Bson::copy_to_bson(char *ptr) const {
        size_t sz = size();
        switch(type()) {
            case k_bson_document:
            case k_bson_array:
                sz = size();
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    Bson_node_type t = iter->second->type();
                    memcpy(ptr++, &t, 1);
                    memcpy(ptr, iter->first.c_str(), iter->first.size() + 1);
                    ptr += iter->first.size() + 1;
                    ptr += iter->second->copy_to_bson(ptr);
                }
                *ptr = 0;
                break;
            default:
                memcpy(ptr, _value, sz);
                break;
        }
        return sz;
    }
    
    std::set<std::string> Bson::children() const {
        std::set<std::string> f;
        if(nested()) {
            for(childmap_t::const_iterator iter = _children.begin();
                iter != _children.end();
                ++iter) {
                if(iter->second->exists())
                    f.insert(iter->first);
            }
        }
        return f;
    }
    
    Bson &Bson::child(const std::string &n) {
        childmap_t::iterator iter = _children.find(n);
        if(iter != _children.end())
            return *(iter->second);
        Bson *ptr = new Bson();
        _children.insert(std::pair<std::string, Bson *>(n, ptr));
        return *ptr;
    }
    
    const Bson &Bson::child(const std::string &n) const {
        childmap_t::const_iterator iter = _children.find(n);
        if(iter == _children.end())
            throw new Exception("DocumentError", std::string("Unable to find child [").append(n).append("]."));
        return *(iter->second);
    }

    namespace {
        void split_path(const std::string &path, std::list<std::string> &parts) {
            const char *tmp = path.c_str();
            std::string current;
            for(;*tmp; ++tmp) {
                if(*tmp == '/') {
                    if(current.size() > 0) {
                        parts.push_back(current);
                        current.erase();
                    }
                } else if(*tmp == '\\' && *(tmp + 1)) {
                    current.push_back(*(++tmp));
                } else {
                    current.push_back(*tmp);
                }
            }
            if(current.size() > 0)
                parts.push_back(current);
        }
        const Bson &navigate_document(const Bson &n, std::list<std::string> &p) {
            if(p.size() < 1)
                return n;
            std::string front = p.front();
            p.pop_front();
            return navigate_document(n.child(front), p);
        }
        Bson &navigate_document(Bson &n, std::list<std::string> &p) {
            if(p.size() < 1)
                return n;
            std::string front = p.front();
            p.pop_front();
            return navigate_document(n.child(front), p);
        }
    };
    
    Bson &Bson::nav(const std::string &p) {
        std::list<std::string> parts;
        split_path(p, parts);
        return navigate_document(*this, parts);
    }
    
    const Bson &Bson::nav(const std::string &p) const {
        std::list<std::string> parts;
        split_path(p, parts);
        return navigate_document(*this, parts);
    }
    
    std::string Bson::type_string() const {
        switch(_type) {
            case k_bson_string:
                return "string";
            case k_bson_int32:
                return "int32";
            case k_bson_double:
                return "double";
            case k_bson_int64:
                return "int64";
            case k_bson_timestamp:
                return "timestamp";
            case k_bson_boolean:
                return "bool";
            case k_bson_null:
                return "null";
            case k_bson_document:
                return "document";
            case k_bson_array:
                return "array";
            default:
                break;
        }
        return "unknown";
    }
    
    bool Bson::exists() const {
        return _children.size() ? true : (_value ? true : false);
    }
    
    bool Bson::nested() const {
        return (_type == k_bson_document || _type == k_bson_array);
    }
    
    bool Bson::quotable() const {
        return (_type == k_bson_string);
    }
    
    size_t Bson::size() const {
        long sz = 0;
        switch(_type) {
            case k_bson_string:
                memcpy(&sz, _value, 4);
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
                for(std::map<std::string, Bson *>::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    sz += iter->second->size() + iter->first.size() + 2;
                }
                break;
            default:
                break;
        }
        return sz;
    }
    
    const Bson &Bson::save(const std::string &fn) const {
        std::ofstream f(fn.c_str());
        char *ptr = bson();
        f.write(ptr, size());
        f.close();
        delete[] ptr;
        return *this;
    }
    
    Bson &Bson::load(const std::string &fn) {
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