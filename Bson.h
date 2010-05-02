#pragma once
/*!
 \file Bson.h
 \brief Bson header file.
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
#include <vector>
#include "Exception.h"

namespace lj {
    //! Enumeration of Document Node Types.
    enum Bson_node_type
    {
        k_bson_double = 0x01,    //!< Node contains a double value.
        k_bson_string = 0x02,    //!< Node contains a string value.
        k_bson_document = 0x03,  //!< Node contains a nested document value.
        k_bson_array = 0x04,     //!< Node contains a nested array value.
        k_bson_binary = 0x05,    //!< Node contains a binary value.
        k_bson_boolean = 0x08,   //!< Node contains a boolean value.
        k_bson_datetime = 0x09,  //!< Node contains a date/time value.
        k_bson_null = 0x0A,      //!< Node contains a null value.
        k_bson_javascript = 0x0D, //!< Node contains a javascript value.
        k_bson_int32 = 0x10,     //!< Node contains a int32 number value.
        k_bson_timestamp = 0x11, //!< Node contains a timestamp value.
        k_bson_int64 = 0x12,     //!< Node contains a int64 number value.
        k_bson_minkey = 0xFF,    //!< Node contains a reserved BSON spec value.
        k_bson_maxkey = 0x7F     //!< Node contains a reserved BSON spec value.
    };
    
    //! Bson document element.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class Bson {
    private:
        std::map<std::string, Bson*> child_map_;
        std::vector<std::pair<std::string, Bson *> > child_array_;
        char* value_;
        Bson_node_type type_;
    public:
        //=====================================================================
        // DocumentNode ctor/dtor
        //=====================================================================
        
        //! Create a new document Node.
        Bson();
        
        //! Create a new document node based on some data.
        Bson(const Bson_node_type t, const char* v);
        
        //! Create a new document node as a copy.
        Bson(const Bson &o);
        
        //! Destructor.
        ~Bson();
        
        //=====================================================================
        // Bson Instance
        //=====================================================================            
        
        //---------------------------------------------------------------------
        // Bson value setters.
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
        Bson& set_value(const Bson_node_type t, const char* v);
        //! Set the value of the document node to a string value.
        Bson& value(const std::string& v);
        //! Set the value of the document node to a int value.
        Bson& value(const int v);
        //! Set the value of the document node to a long long value.
        Bson& value(const long long v);
        //! Set the value of the document node to a double value.
        Bson& value(const double v);
        //! Set the value of the document node to a boolean value.
        Bson& value(const bool v);
        //! Set the value of the document node to null.
        /*!
         \par
         Nullified nodes exist, but do not contain a value.
         \return Reference to \c this .
         */
        Bson &nullify();
        //! Set the value of the document node to not exist.
        /*!
         \par
         Destroyed values no longer exist, and have no value.
         \return Reference to \c this .
         */
        Bson &destroy();
        //! set or create a child of this node.
        /*!
         \par
         Destroy (and \c delete) the previous child \c n , and replace it with a
         copy of \c c .
         \param n The name of the child to replace.
         \param c The child to copy from.
         \return Reference to \c this .
         */
        Bson& child(const std::string& n, const Bson& c);
        Bson& assign(const Bson& o);
        Bson& operator=(const Bson& o) { return assign(o); };

        //---------------------------------------------------------------------
        // Bson value getters.
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
        //! get the value of the document node as a pretty string.
        std::string to_pretty_s(int lvl = 0) const;
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
        char* bson() const;
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
        const std::map<std::string, Bson*>& to_map() const { return child_map_; };
        //! get a specific child of this node.
        /*!
         \par
         Creates the child if it does not exist.
         \param n The name of the child to get.
         \return Reference to the child.
         */
        Bson& child(const std::string& n);
        //! get a specific child of this node.
        /*!
         \par
         Throws an exception if the child does not exist.
         \param n The name of the child to get.
         \return Reference to the child.
         \throws Exception if the child does not exist.
         */
        const Bson& child(const std::string& n) const;
        //! navigate to a specific child.
        Bson& nav(const std::string& p);
        //! navigate to a specific child.
        const Bson& nav(const std::string& p) const;
        
        //---------------------------------------------------------------------
        // DocumentNode inspectors.
        //---------------------------------------------------------------------
        
        //! Get the type of the document node.
        Bson_node_type type() const { return type_; }
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
        const Bson& save(const std::string& fn) const;
        //! Load this document node from disk.
        Bson& load(const std::string& fn);
    };
};
