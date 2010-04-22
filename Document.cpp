/*
 \file Document.cpp
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

#include "Document.h"
#include "Exception.h"
#include <cstring>
#include <list>
#include <sstream>
#include <iostream>
#include <fstream>

namespace tokyo {
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
        
        void subdocument(DocumentNode &node, const void *value) {
            // treat it as a char * for pointer math reasons.
            const char *ptr = (const char *)value;
            
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
                node.child(name, DocumentNode((DocumentNodeType)t, ptr));
                sz = 0;
                switch((DocumentNodeType)t) {
                    case STRING_NODE:
                        memcpy(&sz, ptr, 4);
                        sz += 4;
                        break;
                    case INT32_NODE:
                        sz = 4;
                        break;
                    case DOUBLE_NODE:
                    case INT64_NODE:
                    case TIMESTAMP_NODE:
                        sz = 8;
                        break;
                    case BOOL_NODE:
                        sz = 1;
                        break;
                    case DOC_NODE:
                    case ARRAY_NODE:
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
    // DocumentNode Lua integration
    //=====================================================================
    const char DocumentNode::LUNAR_CLASS_NAME[] = "DocumentNode";
    
    Lunar<DocumentNode>::RegType DocumentNode::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(DocumentNode, at),
    LUNAR_MEMBER_METHOD(DocumentNode, set),
    {0, 0}
    };
    
    int DocumentNode::_at(lua_State *L) {
        std::string name(luaL_checkstring(L, -1));
        switch(child(name).type()) {
            case DOC_NODE:
            case ARRAY_NODE:
                Lunar<DocumentNode>::push(L, &(child(name)), false);
                break;
            case INT32_NODE:
            case INT64_NODE:
                lua_pushinteger(L, child(name).to_long());
                break;
            case STRING_NODE:
                lua_pushstring(L, child(name).to_str().c_str());
                break;
            case DOUBLE_NODE:
            default:
                lua_pushnil(L);
                break;
        }
        return 1;
    }
    
    int DocumentNode::_set(lua_State *L) {
        size_t len = 0;
        const char *str;
        int tmp;
        lua_settop(L, 1);
        switch(lua_type(L, 1)) {
            case LUA_TSTRING:
                str = lua_tolstring(L, 1, &len);
                value(std::string(str));
                break;
            case LUA_TNUMBER:
                value(lua_tointeger(L, 1));
                break;
            case LUA_TNIL:
                value(NULL_NODE, NULL);
                break;
            case LUA_TBOOLEAN:
                tmp = lua_toboolean(L, 1) ? 1 : 0;
                value(BOOL_NODE, &tmp);
                break;
            case LUA_TTABLE:
            case LUA_TFUNCTION:
            case LUA_TTHREAD:
            case LUA_TUSERDATA:
            case LUA_TLIGHTUSERDATA:
            case LUA_TNONE:
            default:
                break;
        }
        return 0;
    }
    
    //=====================================================================
    // DocumentNode ctor/dtor
    //=====================================================================
    
    DocumentNode::DocumentNode(lua_State *L) : _children(), _value(NULL), _type(DOC_NODE) {
        // Add some logic to distinguish copy constructor from empty constructor.
    }
                                            
    DocumentNode::DocumentNode() : _children(), _value(NULL), _type(DOC_NODE) {
    }
    
    DocumentNode::DocumentNode(const DocumentNodeType t, const void *v) : _children(), _value(NULL), _type(DOC_NODE) {
        value(t, v);
    }
    
    DocumentNode::DocumentNode(const DocumentNode &o) : _children(), _value(NULL), _type(o._type) {
        if(o.nested()) {
            for(std::map<std::string, DocumentNode *>::const_iterator iter = o._children.begin();
                iter != o._children.end();
                ++iter) {
                DocumentNode *ptr = new DocumentNode(*(iter->second));
                _children.insert(std::pair<std::string, DocumentNode *>(iter->first, ptr));
            }
        } else {
            value(o._type, o._value);
        }
    }
    
    DocumentNode::~DocumentNode() {
        if(_value)
            free(_value);
        for(childmap_t::const_iterator iter = _children.begin();
            iter != _children.end();
            ++iter) {
            delete iter->second;
        }
    }
    
    //=====================================================================
    // DocumentNode Instance
    //=====================================================================
    
    DocumentNode &DocumentNode::value(const DocumentNodeType t, const void *v) {
        // assume the type may have changed.
        void *old = NULL;
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
                case STRING_NODE:
                    memcpy(&sz, v, 4);
                    _value = malloc(sz + 4);
                    memcpy(_value, v, sz + 4);
                    break;
                case INT32_NODE:
                    _value = malloc(4);
                    memcpy(_value, v, 4);
                    break;
                case DOUBLE_NODE:
                case INT64_NODE:
                case TIMESTAMP_NODE:
                    _value = malloc(8);
                    memcpy(_value, v, 8);
                    break;
                case BOOL_NODE:
                    _value = malloc(1);
                    memcpy(_value, v, 1);
                    break;
                case NULL_NODE:
                    _value = NULL;
                    break;
                case DOC_NODE:
                case ARRAY_NODE:
                    memcpy(&sz, v, 4);
                    subdocument(*this, v);
                    break;
                default:
                    break;
            }
        }
        
        // Clean up any old memory.
        if(old)
            free(old);
        
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const std::string &v) {
        long sz = v.size() + 1;
        char *ptr = (char *)malloc(sz + 4);
        memcpy(ptr, &sz, 4);
        memcpy(ptr + 4, v.c_str(), sz);
        value(STRING_NODE, ptr);
        free(ptr);
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const int v) {
        char *ptr = (char *)malloc(4);
        memcpy(ptr, &v, 4);
        value(INT32_NODE, ptr);
        free(ptr);
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const long long v) {
        char *ptr = (char *)malloc(8);
        memcpy(ptr, &v, 8);
        value(INT64_NODE, ptr);
        free(ptr);
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const double v) {
        char *ptr = (char *)malloc(8);
        memcpy(ptr, &v, 8);
        value(DOUBLE_NODE, ptr);
        free(ptr);
        return *this;
    }
    
    std::string DocumentNode::type_string() const {
        switch(_type) {
            case STRING_NODE:
                return "string";
            case INT32_NODE:
                return "int32";
            case DOUBLE_NODE:
                return "double";
            case INT64_NODE:
                return "int64";
            case TIMESTAMP_NODE:
                return "timestamp";
            case BOOL_NODE:
                return "bool";
            case NULL_NODE:
                return "null";
            case DOC_NODE:
                return "document";
            case ARRAY_NODE:
                return "array";
            default:
                break;
        }
        return "unknown";
    }
    
    bool DocumentNode::exists() const {
        return _children.size() ? true : (_value ? true : false);
    }
    
    bool DocumentNode::nested() const {
        return (_type == DOC_NODE || _type == ARRAY_NODE);
    }
    
    size_t DocumentNode::size() const {
        long sz = 0;
        switch(_type) {
            case STRING_NODE:
                memcpy(&sz, _value, 4);
                sz += 4;
                break;
            case INT32_NODE:
                sz = 4;
                break;
            case DOUBLE_NODE:
            case INT64_NODE:
            case TIMESTAMP_NODE:
                sz = 8;
                break;
            case BOOL_NODE:
                sz = 1;
                break;
            case NULL_NODE:
                sz = 0;
                break;
            case DOC_NODE:
            case ARRAY_NODE:
                sz += 5;
                for(std::map<std::string, DocumentNode *>::const_iterator iter = _children.begin();
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
    
    std::string DocumentNode::to_debug_str() const {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch(_type) {
            case STRING_NODE:
                memcpy(&l, _value, 4);
                buf << "(4-" << l << ")" << "(" << l << ")" << (((char *)_value) + 4);
                return buf.str();
            case INT32_NODE:
                memcpy(&l, _value, 4);
                buf << "(4)" << l;
                return buf.str();
            case DOUBLE_NODE:
                memcpy(&d, _value, 8);
                buf << "(8)" << d;
                return buf.str();
            case INT64_NODE:
            case TIMESTAMP_NODE:
                memcpy(&l, _value, 8);
                buf << "(8)" << l;
                return buf.str();
            case BOOL_NODE:
                memcpy(&l, _value, 1);
                buf << "(1)" << ((bool)l);
                return buf.str();
            case DOC_NODE:
            case ARRAY_NODE:
                if(!_children.size())
                    return "{(4-0)(1-0)}";
                buf << "{(4-" << size() << ")";
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    buf << "(1-" << iter->second->type_string() << ")";
                    buf << "\"(" << iter->first.size() + 1 << ")" << escape(iter->first) << "\":";
                    if(iter->second->type() == STRING_NODE)
                        buf << "\"";
                    buf << iter->second->to_str();
                    if(iter->second->type() == STRING_NODE)
                        buf << "\"";
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("(1-0)}");
            default:
                break;
        }
        return std::string();
    }

    std::string DocumentNode::to_str() const {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch(_type) {
            case STRING_NODE:
                memcpy(&l, _value, 4);
                return (((char *)_value) + 4);
            case INT32_NODE:
                memcpy(&l, _value, 4);
                buf << l;
                return buf.str();
            case DOUBLE_NODE:
                memcpy(&d, _value, 8);
                buf << d;
                return buf.str();
            case INT64_NODE:
            case TIMESTAMP_NODE:
                memcpy(&l, _value, 8);
                buf << l;
                return buf.str();
            case BOOL_NODE:
                memcpy(&l, _value, 1);
                buf << ((bool)l);
                return buf.str();
            case DOC_NODE:
            case ARRAY_NODE:
                if(!_children.size())
                    return "{}";
                buf << "{";
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    buf << "\"" << escape(iter->first) << "\":";
                    if(iter->second->type() == STRING_NODE)
                        buf << "\"";
                    buf << iter->second->to_str();
                    if(iter->second->type() == STRING_NODE)
                        buf << "\"";
                    buf << ",";
                }
                return buf.str().erase(buf.str().size() - 1).append("}");
            default:
                break;
        }
        return std::string();
    }
    
    std::set<std::string> DocumentNode::to_str_set() const {
        std::set<std::string> f;
        switch(_type) {
            case DOC_NODE:
            case ARRAY_NODE:
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    if(iter->second->exists()) 
                        f.insert(iter->second->to_str());
                }
                break;
            default:
                if(exists())
                    f.insert(to_str());
                break;
        }
        return f;
    }
    
    int DocumentNode::to_int() const {
        long l = 0;
        double d = 0.0;
        switch(_type) {
            case STRING_NODE:
                return atoi(((char *)_value) + 4);
            case INT32_NODE:
                memcpy(&l, _value, 4);
                return (int)l;
            case DOUBLE_NODE:
                memcpy(&d, _value, 8);
                return (int)d;
            case INT64_NODE:
            case TIMESTAMP_NODE:
                memcpy(&l, _value, 8);
                return (int)l;
            case BOOL_NODE:
                memcpy(&l, _value, 1);
                return (int)l;
            default:
                break;
        }
        return 0;
    }
    
    long long DocumentNode::to_long() const {
        long l = 0;
        double d = 0.0;
        switch(_type) {
            case STRING_NODE:
                return atol(((char *)_value) + 4);
            case INT32_NODE:
                memcpy(&l, _value, 4);
                return l;
            case DOUBLE_NODE:
                memcpy(&d, _value, 8);
                return (long long)d;
            case INT64_NODE:
            case TIMESTAMP_NODE:
                memcpy(&l, _value, 8);
                return l;
            case BOOL_NODE:
                memcpy(&l, _value, 1);
                return l;
            default:
                break;
        }
        return 0;
    }
    
    bool DocumentNode::to_bool() const  {
        long l = 0;
        double d = 0.0;
        char *s = ((char *)_value) + 4;
        switch(_type) {
            case STRING_NODE:
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
            case INT32_NODE:
                memcpy(&l, _value, 4);
                return l;
            case DOUBLE_NODE:
                memcpy(&d, _value, 8);
                return (long)d;
            case INT64_NODE:
            case TIMESTAMP_NODE:
                memcpy(&l, _value, 8);
                return l;
            case BOOL_NODE:
                memcpy(&l, _value, 1);
                return l;
            default:
                break;
        }
        return false;
    }
    
    void * DocumentNode::to_bson() const {
        char *ptr, *real_ptr;
        void *sub_doc;
        long long sz;
        switch(_type) {
            case DOC_NODE:
            case ARRAY_NODE:
                sz = size();
                real_ptr = (char *)malloc(sz);
                ptr = real_ptr;
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    DocumentNodeType t = iter->second->type();
                    memcpy(ptr++, &t, 1);
                    memcpy(ptr, iter->first.c_str(), iter->first.size() + 1);
                    ptr += iter->first.size() + 1;
                    sub_doc = iter->second->to_bson();
                    memcpy(ptr, sub_doc, iter->second->size());
                    if(t == DOC_NODE || t == ARRAY_NODE)
                        free(sub_doc);
                    ptr += iter->second->size();
                }
                *ptr = 0;
                return real_ptr;
            default:
                return _value;
        }
    }

    DocumentNode &DocumentNode::child(const std::string &n) {
        childmap_t::iterator iter = _children.find(n);
        if(iter != _children.end())
            return *(iter->second);
        DocumentNode *ptr = new DocumentNode();
        _children.insert(std::pair<std::string, DocumentNode *>(n, ptr));
        return *ptr;
    }
    
    const DocumentNode &DocumentNode::child(const std::string &n) const {
        childmap_t::const_iterator iter = _children.find(n);
        if(iter == _children.end())
            throw Exception("DocumentError", std::string("Unable to find child [").append(n).append("]."));
        return *(iter->second);
    }
    
    DocumentNode &DocumentNode::child(const std::string &n, const DocumentNode &c) {
        childmap_t::iterator iter = _children.find(n);
        DocumentNode *ptr = new DocumentNode(c);
        if(iter != _children.end()) {
            delete iter->second;
            _children.erase(iter);
        }
        
        _children.insert(std::pair<std::string, DocumentNode *>(n, ptr));
        return *ptr;
    }

    //=====================================================================
    // Document Lua integration
    //=====================================================================
    const char Document::LUNAR_CLASS_NAME[] = "Document";
    
    Lunar<Document>::RegType Document::LUNAR_METHODS[] = {
    LUNAR_MEMBER_METHOD(Document, at),
    LUNAR_MEMBER_METHOD(Document, save),
    LUNAR_MEMBER_METHOD(Document, load),
    {0, 0}
    };
    
    int Document::_at(lua_State *L) {
        return _doc->_at(L);
    }
    
    int Document::_save(lua_State *L) {
        std::string n(luaL_checkstring(L, -1));
        save(n);
        return 0;
    }
    
    int Document::_load(lua_State *L) {
        std::string n(luaL_checkstring(L, -1));
        load(n);
        return 0;
    }
    
    //=====================================================================
    // Document ctor/dtor
    //=====================================================================
    
    Document::Document(lua_State *L) : _doc(NULL) {
        _doc = new DocumentNode(DOC_NODE, NULL);
    }
    
    Document::Document() : _doc(NULL) {
        _doc = new DocumentNode(DOC_NODE, NULL);
    }
    
    Document::Document(const DB::value_t &p) : _doc(NULL) {
        _doc = new DocumentNode(DOC_NODE, NULL);
        from_db_value(p);
    }
    
    Document::Document(const Document &orig) : _doc(NULL) {
        _doc = new DocumentNode(*(orig._doc));
    }
    
    Document::~Document() {
        if(_doc) delete _doc;
    }
    
    //=====================================================================
    // Document Instance
    //=====================================================================
    
    Document &Document::swap(Document &d) {
        DocumentNode *old = _doc;
        _doc = d._doc;
        d._doc = old;
        return *this;
    }
    
    DB::value_t Document::to_db_value() const {
        
        std::cerr << "to " << _doc->to_str() << std::endl;
        return DB::value_t(_doc->to_bson(), _doc->size());
    }
    
    Document &Document::from_db_value(const DB::value_t &p) {
        DocumentNode *doc = new DocumentNode(DOC_NODE, p.first);
        
        std::cerr << "from " << doc->to_str() << std::endl;
        
        if(doc->size() != p.second) {
            delete doc;
            throw Exception("DocumentError", std::string("Provided document information does not match the resulting document"));
        }
        delete _doc;
        _doc = doc;
        return *this;
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
        const DocumentNode &navigate_document(const DocumentNode &n, std::list<std::string> &p) {
            if(p.size() < 1)
                return n;
            std::string front = p.front();
            p.pop_front();
            return navigate_document(n.child(front), p);
        }
        DocumentNode &navigate_document(DocumentNode &n, std::list<std::string> &p) {
            if(p.size() < 1)
                return n;
            std::string front = p.front();
            p.pop_front();
            return navigate_document(n.child(front), p);
        }
    };
    
    const DocumentNode &Document::path(const std::string &path) const {
        std::list<std::string> parts;
        split_path(path, parts);
        return navigate_document(*_doc, parts);
    }
    
    Document &Document::path(const std::string &path, const std::string &v) {
        std::list<std::string> parts;
        split_path(path, parts);
        navigate_document(*_doc, parts).value(v);
        return *this;
    }
    
    Document &Document::path(const std::string &path, const int v) {
        std::list<std::string> parts;
        split_path(path, parts);
        navigate_document(*_doc, parts).value(v);
        return *this;
    }
    
    Document &Document::path(const std::string &path, const long long v) {
        std::list<std::string> parts;
        split_path(path, parts);
        navigate_document(*_doc, parts).value(v);
        return *this;
    }
    
    Document &Document::path(const std::string &path, const double v) {
        std::list<std::string> parts;
        split_path(path, parts);
        navigate_document(*_doc, parts).value(v);
        return *this;
    }
    Document &Document::path(const std::string &path, const std::string &child, const DocumentNode &v) {
        std::list<std::string> parts;
        split_path(path, parts);
        navigate_document(*_doc, parts).child(child, v);
        return *this;
    }
    Document &Document::load(const std::string &filename) {
        std::ifstream f(filename.c_str());
        long sz = 0, offset = 4;
        
        f.read((char *)(&sz), 4);
        char *data = new char[sz];
        memcpy(data, &sz, 4);
        
        while(offset < sz) {
            f.read((data + offset), sz - offset);
            offset += f.gcount();
        }
        _doc->value(DOC_NODE, data);
        
        delete[] data;
        f.close();
        return *this;
    }
    Document &Document::save(const std::string &filename) {
        std::ofstream f(filename.c_str());
        long sz = _doc->size();
        
        char *data = (char *)_doc->to_bson();
        f.write(data, sz);
        
        free(data);
        f.close();
        return *this;
    }
};