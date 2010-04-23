#pragma once
/*
 \file DocumentNode.h
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

#include <list>
#include <map>
#include <set>
#include <string>
#include "lunar.h"
#include "Exception.h"

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
        char *_value;
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
        
        int _nav(lua_State *L);
        
        int _set(lua_State *L);
        
        int _get(lua_State *L);
        
        int _save(lua_State *L);
        
        int _load(lua_State *L);
        
        //=====================================================================
        // DocumentNode ctor/dtor
        //=====================================================================
        
        //! Create a new document Node.
        DocumentNode();
        
        //! Create a new document node based on some data.
        DocumentNode(const DocumentNodeType t, const char *v);
        
        //! Create a new document node as a copy.
        DocumentNode(const DocumentNode &o);
        
        //! Destructor.
        ~DocumentNode();
        
        //=====================================================================
        // DocumentNode Instance
        //=====================================================================            
        
        //---------------------------------------------------------------------
        // DocumentNode value setters.
        //---------------------------------------------------------------------
        
        //! Set the value of the document node based on a bson string.
        /*!
         \par
         The value of v is copied out of the pointer \c v , and must be freed
         by the calling application.
         \param t The new type of the document.
         \param v Array of data to read the new value from.
         \return Reference to \c this .
         */
        DocumentNode &set_value(const DocumentNodeType t, const char *v);
        //! Set the value of the document node to a string value.
        DocumentNode &value(const std::string &v);
        //! Set the value of the document node to a int value.
        DocumentNode &value(const int v);
        //! Set the value of the document node to a long long value.
        DocumentNode &value(const long long v);
        //! Set the value of the document node to a double value.
        DocumentNode &value(const double v);
        //! Set the value of the document node to null.
        /*!
         \par
         Nullified nodes exist, but do not contain a value.
         \return Reference to \c this .
         */
        DocumentNode &nullify();
        //! Set the value of the document node to not exist.
        /*!
         \par
         Destroyed values no longer exist, and have no value.
         \return Reference to \c this .
         */
        DocumentNode &destroy();
        //! set or create a child of this node.
        /*!
         \par
         Destroy (and \c delete) the previous child \c n , and replace it with a
         copy of \c c .
         \param n The name of the child to replace.
         \param c The child to copy from.
         \return Reference to \c this .
         */
        DocumentNode &child(const std::string &n, const DocumentNode &c);

        //---------------------------------------------------------------------
        // DocumentNode value getters.
        //---------------------------------------------------------------------
        
        //! get the value of the document node as a debug string.
        /*!
         \par
         The debug string is a representation of the DocumentNode in BSON,
         format.  Rather than being a byte array, the results are output in
         a psudeo-JSON looking format, with lengths and byte counts included
         in the display.
         \par
         This is really only useful for debugging output.
         \return A string describing how this DocumentNode should look in BSON.
         */
        std::string to_dbg_s() const;
        //! get the value of the document node as a string.
        std::string to_s() const;
        //! get the value of the children of a document node as a set of strings.
        std::set<std::string> to_set() const;
        //! get the value of the children of a document node as a list of strings.
        std::list<std::string> to_list() const;
        //! get the value of the document node as an int.
        int to_i() const;
        //! get the value of the document node as a long long.
        long long to_l() const;
        //! get the value of the document node as a boolean.
        bool to_b() const;
        //! get the value of the document node as a double.
        double to_d() const;
        //! get the value of the document node as a bson string.
        /*!
         \par
         Pointer is allocated with "new[]" and must be released with "delete[]".
         \par
         The array length can be obtained by calling \c size() .
         \par
         The bson bytes include "empty" documents that may not appear in
         \c to_s(), but will appear in \c to_dbg_s() .
         \return A byte array contain the bson document.
         */
        char *bson() const;
    private:
        //! copy the value of this object into a bson byte array.
        size_t copy_to_bson(char *) const;
    public:
        
        //---------------------------------------------------------------------
        // DocumentNode child getters.
        //---------------------------------------------------------------------
        
        //! get the keys of all the children of this node.
        std::set<std::string> children() const;
        //! Get the children of this node.
        const childmap_t &to_map() const { return _children; };
        //! get the children of this node.
        childmap_t &to_map() { return _children; };
        //! get a specific child of this node.
        /*!
         \par
         Creates the child if it does not exist.
         \param n The name of the child to get.
         \return Reference to the child.
         */
        DocumentNode &child(const std::string &n);
        //! get a specific child of this node.
        /*!
         \par
         Throws an exception if the child does not exist.
         \param n The name of the child to get.
         \return Reference to the child.
         \throws Exception if the child does not exist.
         */
        const DocumentNode &child(const std::string &n) const;
        //! navigate to a specific child.
        DocumentNode &nav(const std::string &p);
        //! navigate to a specific child.
        const DocumentNode &nav(const std::string &p) const;
        
        //---------------------------------------------------------------------
        // DocumentNode inspectors.
        //---------------------------------------------------------------------
        
        //! Get the type of the document node.
        DocumentNodeType type() const { return _type; }
        //! Get a string version of the type.
        std::string type_string() const;
        //! Get if the node actually exists.
        bool exists() const;
        //! Get if the node is a nested node type.
        bool nested() const;
        //! Get if the node is a string type.
        bool quotable() const;
        //! Get the size of the node.
        size_t size() const;
        
        //---------------------------------------------------------------------
        // File System Helpers
        //---------------------------------------------------------------------
        
        //! Save this document node to disk.
        const DocumentNode &save(const std::string &fn) const;
        //! Load this document node from disk.
        DocumentNode &load(const std::string &fn);
    };
};
