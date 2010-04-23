/*
 *  DocumentNode.cpp
 *  logjammin
 *
 *  Created by Jason Watson on 4/22/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "build/default/config.h"
#include "DocumentNode.h"
#include "Exception.h"
#include <cstring>
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
        
        void subdocument(DocumentNode &node, const char *value) {
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
    LUNAR_MEMBER_METHOD(DocumentNode, nav),
    LUNAR_MEMBER_METHOD(DocumentNode, set),
    LUNAR_MEMBER_METHOD(DocumentNode, get),
    LUNAR_MEMBER_METHOD(DocumentNode, load),
    LUNAR_MEMBER_METHOD(DocumentNode, save),
    {0, 0}
    };
    
    int DocumentNode::_nav(lua_State *L) {
        std::string path(luaL_checkstring(L, -1));
        Lunar<DocumentNode>::push(L, &(nav(path)), false);
        return 1;
    }
    
    int DocumentNode::_set(lua_State *L) {
        const char *str;
        int tmp;
        lua_settop(L, 1);
        switch(lua_type(L, 1)) {
            case LUA_TSTRING:
                str = luaL_checkstring(L, 1);
                value(std::string(str));
                break;
            case LUA_TNUMBER:
                value(luaL_checkint(L, 1));
                break;
            case LUA_TNIL:
                set_value(NULL_NODE, NULL);
                break;
            case LUA_TBOOLEAN:
                tmp = lua_toboolean(L, 1) ? 1 : 0;
                set_value(BOOL_NODE, (char *)&tmp);
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
    
    int DocumentNode::_get(lua_State *L) {
        switch(type()) {
            case INT32_NODE:
            case INT64_NODE:
            case TIMESTAMP_NODE:
                lua_pushinteger(L, to_l());
                break;
            case DOC_NODE:
            case ARRAY_NODE:
            case STRING_NODE:
                lua_pushstring(L, to_s().c_str());
                break;
            case DOUBLE_NODE:
            default:
                lua_pushnil(L);
                break;
        }
        return 1;
    }
        
    int DocumentNode::_save(lua_State *L) {
        std::string fn(luaL_checkstring(L, -1));
        save(fn);
        return 0;
    }
    
    int DocumentNode::_load(lua_State *L) {
        std::string fn(luaL_checkstring(L, -1));
        load(fn);
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
    
    DocumentNode::DocumentNode(const DocumentNodeType t, const char *v) : _children(), _value(NULL), _type(DOC_NODE) {
        set_value(t, v);
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
            set_value(o._type, o._value);
        }
    }
    
    DocumentNode::~DocumentNode() {
        if(_value)
            delete[] _value;
        for(childmap_t::const_iterator iter = _children.begin();
            iter != _children.end();
            ++iter) {
            delete iter->second;
        }
    }
    
    //=====================================================================
    // DocumentNode Instance
    //=====================================================================
    
    DocumentNode &DocumentNode::set_value(const DocumentNodeType t, const char *v) {
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
                case STRING_NODE:
                    memcpy(&sz, v, 4);
                    _value = new char[sz + 4];
                    memcpy(_value, v, sz + 4);
                    break;
                case INT32_NODE:
                    _value = new char[4];
                    memcpy(_value, v, 4);
                    break;
                case DOUBLE_NODE:
                case INT64_NODE:
                case TIMESTAMP_NODE:
                    _value = new char[8];
                    memcpy(_value, v, 8);
                    break;
                case BOOL_NODE:
                    _value = new char[1];
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
            delete[] old;
        
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const std::string &v) {
        long sz = v.size() + 1;
        char *ptr = new char[sz + 4];;
        memcpy(ptr, &sz, 4);
        memcpy(ptr + 4, v.c_str(), sz);
        set_value(STRING_NODE, ptr);
        delete[] ptr;
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const int v) {
        char ptr[4];
        memcpy(ptr, &v, 4);
        set_value(INT32_NODE, ptr);
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const long long v) {
        char ptr[8];
        memcpy(ptr, &v, 8);
        set_value(INT64_NODE, ptr);
        return *this;
    }
    
    DocumentNode &DocumentNode::value(const double v) {
        char ptr[8];
        memcpy(ptr, &v, 8);
        set_value(DOUBLE_NODE, ptr);
        return *this;
    }
    
    DocumentNode &DocumentNode::nullify() {
        set_value(NULL_NODE, NULL);
        return *this;
    }

    DocumentNode &DocumentNode::destroy() {
        set_value(DOC_NODE, NULL);
        return *this;
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
    
    std::string DocumentNode::to_dbg_s() const {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch(type()) {
            case STRING_NODE:
                memcpy(&l, _value, 4);
                buf << "(4-" << l << ")" << "(" << l << ")" << (_value + 4);
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

    std::string DocumentNode::to_s() const {
        long long l = 0;
        double d = 0.0;
        std::ostringstream buf;
        switch(type()) {
            case STRING_NODE:
                memcpy(&l, _value, 4);
                return std::string(_value + 4);
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
            case NULL_NODE:
                return std::string("null");
            case DOC_NODE:
            case ARRAY_NODE:
                if(!_children.size())
                    return "{}";
                buf << "{";
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    if(!iter->second->exists())
                        continue;
                    buf << "\"" << escape(iter->first) << "\":";
                    if(iter->second->type() == STRING_NODE)
                        buf << "\"";
                    buf << iter->second->to_s();
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
    
    std::set<std::string> DocumentNode::to_set() const {
        std::set<std::string> f;
        switch(type()) {
            case DOC_NODE:
            case ARRAY_NODE:
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
    
    std::list<std::string> DocumentNode::to_list() const {
        std::list<std::string> f;
        switch(type()) {
            case DOC_NODE:
            case ARRAY_NODE:
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
    
    int DocumentNode::to_i() const {
        long l = 0;
        double d = 0.0;
        switch(type()) {
            case STRING_NODE:
                return atoi(_value + 4);
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
    
    long long DocumentNode::to_l() const {
        long l = 0;
        double d = 0.0;
        switch(type()) {
            case STRING_NODE:
                return atol(_value + 4);
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
    
    bool DocumentNode::to_b() const  {
        long l = 0;
        double d = 0.0;
        char *s = _value + 4;
        switch(type()) {
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
    
    double DocumentNode::to_d() const {
        long l = 0;
        double d = 0.0;
        switch(type()) {
            case STRING_NODE:
                return atof(_value + 4);
            case INT32_NODE:
                memcpy(&l, _value, 4);
                return (double)l;
            case DOUBLE_NODE:
                memcpy(&d, _value, 8);
                return d;
            case INT64_NODE:
            case TIMESTAMP_NODE:
                memcpy(&l, _value, 8);
                return (double)l;
            case BOOL_NODE:
                memcpy(&l, _value, 1);
                return (double)l;
            default:
                break;
        }
        return 0.0;
    }
    
    char *DocumentNode::bson() const {
        char *ptr = new char[size()];
        copy_to_bson(ptr);
        return ptr;
    }
    
    size_t DocumentNode::copy_to_bson(char *ptr) const {
        size_t sz = size();
        switch(type()) {
            case DOC_NODE:
            case ARRAY_NODE:
                sz = size();
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for(childmap_t::const_iterator iter = _children.begin();
                    iter != _children.end();
                    ++iter) {
                    DocumentNodeType t = iter->second->type();
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
    
    std::set<std::string> DocumentNode::children() const {
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
    
    DocumentNode &DocumentNode::nav(const std::string &p) {
        std::list<std::string> parts;
        split_path(p, parts);
        return navigate_document(*this, parts);
    }
    
    const DocumentNode &DocumentNode::nav(const std::string &p) const {
        std::list<std::string> parts;
        split_path(p, parts);
        return navigate_document(*this, parts);
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
    
    bool DocumentNode::quotable() const {
        return (_type == STRING_NODE);
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
    
    const DocumentNode &DocumentNode::save(const std::string &fn) const {
        std::ofstream f(fn.c_str());
        char *ptr = bson();
        f.write(ptr, size());
        f.close();
        delete[] ptr;
        return *this;
    }
    
    DocumentNode &DocumentNode::load(const std::string &fn) {
        std::ifstream f(fn.c_str());
        size_t sz = 0;
        f.read((char *)&sz, 4);
        char *ptr = new char[sz];
        memcpy(ptr, &sz, 4);
        f.read(ptr + 4, sz - 4);
        set_value(DOC_NODE, ptr);
        f.close();
        delete[] ptr;
        return *this;
    }
};