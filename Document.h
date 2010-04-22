#pragma once
/*
 \file Document.h
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

#include <map>
#include <string>
#include "Exception.h"
#include "Tokyo.h"
#include "lunar.h"

namespace tokyo {
    //! Enumeration of Document Node Types.
    enum DocumentNodeType {
        DOUBLE_NODE = 0x01,
        STRING_NODE = 0x02,
        DOC_NODE = 0x03,
        ARRAY_NODE = 0x04,
        BINARY_NODE = 0x05,
        BOOL_NODE = 0x08,
        DATETIME_NODE = 0x09,
        NULL_NODE = 0x0A,
        JS_NODE = 0x0D,
        INT32_NODE = 0x10,
        TIMESTAMP_NODE = 0x11,
        INT64_NODE = 0x12,
        MINKEY_NODE = 0xFF,
        MAXKEY_NODE = 0x7F
    };
    
    class DocumentNode;
    
    //! Node in a BSON document.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class DocumentNode {
    private:
        typedef std::map<std::string, DocumentNode *> childmap_t;
        childmap_t _children;
        void *_value;
        DocumentNodeType _type;
    public:
        //=====================================================================
        // DocumentNode Lua Integration
        //=====================================================================
        
        //! Lua bindings class name.
        static const char LUNAR_CLASS_NAME[];
        
        //! Lua bindings method array.
        static Lunar<DocumentNode>::RegType LUNAR_METHODS[];
        
        //! Create a new document node for lua.
        DocumentNode(lua_State *L);
        
        virtual int _child(lua_State *L);
        
        virtual int _set(lua_State *L);
        
        virtual int _val(lua_State *L);
        
        //=====================================================================
        // DocumentNode ctor/dtor
        //=====================================================================
        
        //! Create a new document Node.
        DocumentNode();
        
        //! Create a new document node based on some data.
        DocumentNode(const DocumentNodeType t, const void *v);
        
        //! Create a new document node as a copy.
        DocumentNode(const DocumentNode &o);
        
        //! Destructor.
        virtual ~DocumentNode();
        
        //=====================================================================
        // DocumentNode Instance
        //=====================================================================            
        
        //! Set the value of the document node based on a bson string.
        virtual DocumentNode &value(const DocumentNodeType t, const void *v);
        //! Set the value of the document node to a string value.
        virtual DocumentNode &value(const std::string &v);
        //! Set the value of the document node to a int value.
        virtual DocumentNode &value(const int v);
        //! Set the value of the document node to a long long value.
        virtual DocumentNode &value(const long long v);
        //! Set the value of the document node to a double value.
        virtual DocumentNode &value(const double v);

        //! Get the type of the document node.
        virtual DocumentNodeType type() const { return _type; }
        virtual std::string type_string() const;
        
        //! Get if the node actually exists.
        virtual bool exists() const;
        
        //! Get if the node is a nested node type.
        virtual bool nested() const;
                
        //! Get the size of the node.
        virtual size_t size() const;
        
        virtual std::string to_debug_str() const;
        //! get the value of the document node as a string.
        virtual std::string to_str() const;
        //! get the value of the children of a document node as a set of strings.
        virtual std::set<std::string> to_str_set() const;
        //! get the value of the document node as an int.
        virtual int to_int() const;
        //! get the value of the document node as a long long.
        virtual long long to_long() const;
        //! get the value of the document node as a boolean.
        virtual bool to_bool() const;
        //! get the value of the document node as a bson string.
        virtual void * to_bson() const;
        
        //! Get the children of this node.
        virtual const childmap_t &children() const { return _children; };
        
        //! get a specific child of this node.
        virtual DocumentNode &child(const std::string &n);
        
        //! get a specific child of this node.
        virtual const DocumentNode &child(const std::string &n) const;
        
        //! set or create a child of this node.
        virtual DocumentNode &child(const std::string &n, const DocumentNode &c);
    };

    //! Root of a BSON document.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 9, 2010
     */
    class Document {
        DocumentNode *_doc;
    public:
        //=====================================================================
        // DocumentNode Lua Integration
        //=====================================================================
        
        //! Lua bindings class name.
        static const char LUNAR_CLASS_NAME[];
        
        //! Lua bindings method array.
        static Lunar<Document>::RegType LUNAR_METHODS[];
        
        //! Create a new document node for lua.
        Document(lua_State *L);
        
        virtual int _child(lua_State *L);
        
        virtual int _root(lua_State *L);
        
        virtual int _load(lua_State *L);
        
        virtual int _save(lua_State *L);
        
        //=====================================================================
        // Document ctor/dtor
        //=====================================================================
        
        //! Create a new Document.
        Document();
        //! Create a new document based on a BSON string from the database.
        Document(const DB::value_t &p);
        //! Create a new document as a copy.
        Document(const Document &orig);
        //! Destructor.
        virtual ~Document();
        
        //=====================================================================
        // Document Instance
        //=====================================================================
        
        //! swap root document nodes.
        /*!
         \par
         this is used for speed in certain situations.
         */
        virtual Document &swap(Document &d);
        
        //! serialize for database.
        virtual DB::value_t to_db_value() const;
        //! marshall for usage.
        virtual Document &from_db_value(const DB::value_t &p);
        
        //! Get the root node.
        virtual const DocumentNode &root() const { return *_doc; };
        //! Get the node at a given path.
        /*!
         \par
         Paths are unix style paths. e.g. "/_key".
         */
        virtual const DocumentNode &path(const std::string &path) const;
        
        //! Set a string value at a path.
        virtual Document &path(const std::string &path, const std::string &v);
        //! Set an int value at a path.
        virtual Document &path(const std::string &path, const int v);
        //! Set a long long value at a path.
        virtual Document &path(const std::string &path, const long long v);
        //! Set a double value at a path.
        virtual Document &path(const std::string &path, const double v);
        //! Set a document value at a path.
        virtual Document &path(const std::string &path, const std::string &child, const DocumentNode &v);
        
        //! Get the document primary key.
        unsigned long long key() const {
            return path("_key").to_long();
        }
        //! Set the document primary key.
        void key(unsigned long long k) {
            path("_key", (const long long)k);
        }
        
        //! load the Document object from a specific file path.
        virtual Document &load(const std::string &filename);
        
        //! Save the Document object to a specific file path.
        virtual Document &save(const std::string &filename);
    };
};
