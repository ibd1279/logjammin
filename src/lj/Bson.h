#pragma once
/*!
 \file Bson.h
 \brief LJ Bson header.
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

#include "lj/Exception.h"
#include "lj/Linked_map.h"
#include <set>
#include <string>

namespace lj
{
    //! Bson value.
    /*!
     \par
     Represets a Bson value, including documents and arrays. The following
     examples show different ways to set a value at some path.
     \code
     Bson *string_node = bson_new_string("test"); // using helper methods.
     Bson *int_node = new bson_new_int(Bson::k_int32, 123) // using constructor.
     Bson root_node; // default constructor.
     Bson some_node;
     
     // Copy the value from another node.
     some_node.copy_from(*string_node); // copy_from method
     some_node = *int_node; // assignment operator.
     
     // Access a specific path.
     other_node = root_node.nav("some/path"); // nav method (reference)
     other_node = *root_node.path("some/path"); // path method (pointer)
     other_node = root_node["some/path"]; // array index operator (reference)
     
     // Set a child node at a path.
     root_node["some/path/int"] = *int_node; // index operator (reference)
     root_node.set_child("some/path/int", int_node); // set_child method (pointer).
     root_node["some/path/string"] << *string_node; // shift left operator (reference).
     root_node.push_child("some/path/string", string_node); // push_child method (pointer).
     \endcode
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class Bson {
    public:
        //! Enumeration of Bson Types.
        enum Type
        {
            k_double = 0x01,    //!< Node contains a double value.
            k_string = 0x02,    //!< Node contains a string value.
            k_document = 0x03,  //!< Node contains a nested document value.
            k_array = 0x04,     //!< Node contains a nested array value.
            k_binary = 0x05,    //!< Node contains a binary value.
            k_binary_document = 0x06, //!< Node contains a document that has not been parsed (Raw bytes).
            k_boolean = 0x08,   //!< Node contains a boolean value.
            k_datetime = 0x09,  //!< Node contains a date/time value.
            k_null = 0x0A,      //!< Node contains a null value.
            k_javascript = 0x0D, //!< Node contains a javascript value.
            k_int32 = 0x10,     //!< Node contains a int32 number value.
            k_timestamp = 0x11, //!< Node contains a timestamp value.
            k_int64 = 0x12,     //!< Node contains a int64 number value.
            k_minkey = 0xFF,    //!< Node contains a reserved BSON spec value.
            k_maxkey = 0x7F     //!< Node contains a reserved BSON spec value.
        };
        
        //! Create a new document Node.
        /*!
         \par
         Creates a blank, empty document that considers itself non-existant.
         Used to create document/array nodes.
         */
        Bson();
        
        //! Create a new document node based on some data.
        /*!
         \par
         Create a new bson value based on the provided values. Used to create
         value nodes.
         \par
         Data from \c v is copied to internal objects.
         \param t The type of node being created.
         \param v The value to associate with this node.
         \sa Bson::set_value()
         */
        Bson(const Bson::Type t, const char* v);
        
        //! Create a new document node as a copy of an existing Bson.
        /*!
         \par
         \c Bson \c a(b) is identical to calling \c Bson \c a(); \c a.copy_from(b).
         \param o The original Bson object.
         \sa Bson::copy_from()
         */
        Bson(const Bson &o);
        
        //! Destructor.
        ~Bson();
                
        //! Set the value of the document node based on a bson string.
        /*!
         \par
         The value of v is copied out of the pointer \c v , and must be freed
         by the calling application.
         \param t The new type of the document.
         \param v Array of data to read the new value from.
         \return Reference to \c this .
         */
        void set_value(const Bson::Type t, const char* v);
        
        //! Set the value of the document node to null.
        /*!
         \par
         Nullified nodes exist, but do not contain a value.
         \return Reference to \c this .
         */
        void nullify();
        
        //! Set the value of the document node to not exist.
        /*!
         \par
         Destroyed values no longer exist, and have no value.
         \return Reference to \c this .
         */
        void destroy();
        
        //! Destroy the current value and copy values from another Bson.
        /*!
         \param o The original Bson object.
         */
        Bson& copy_from(const Bson& o);

        //! Destroy the current value and copy values from another Bson.
        /*!
         \par
         \c a \c = \c b is identical to calling \c a.copy_from(b).
         \param o The original Bson object.
         \sa Bson::copy_from()
         */
        void operator=(const Bson& o)
        {
            copy_from(o);
        }
        
        //! Get a pointer to a specific Bson object in the document.
        /*!
         \par
         The intermediate objects in the path are created if they do not exist.
         \param p The path to follow.
         \return Pointer to that object.
         */
        Bson* path(const std::string& p);
        
        //! Get a pointer to a specific Bson object in the document.
        /*!
         \par
         Null is returned if the intermediate objects in the path do not exist.
         \param p The path to follow.
         \return Pointer to that object, null if it cannot be navigated to.
         */
        const Bson* path(const std::string& p) const;
        
        //! Get a specific Bson object at a path.
        /*!
         \par 
         Reference version of \c path(p).
         \sa path(const std::string)
         \param p The path to follow.
         \return Reference to the object at that node.
         */
        inline Bson& nav(const std::string& p)
        {
            return *path(p);
        }
        
        //! Get a specific Bson object at a path.
        /*!
         \par 
         Reference version of \c path(p).
         \sa path(const std::string)const
         \param p The path to follow.
         \return Reference to the object at that node.
         \throw Exception if the node cannot be navigated to.
         */
        inline const Bson& nav(const std::string& p) const
        {
            const Bson* ptr = path(p);
            if (!ptr)
            {
                throw new Exception("Bson", "Unable to navigate to path.");
            }
            return *ptr;
        }
        
        //! Get a specific Bson object at a path.
        /*!
         \par 
         Syntatical sugar for \c nav(p).
         \sa nav(const std::string)
         \param p The path to follow.
         \return Reference to the object at that node.
         */
        inline Bson& operator[](const std::string& p)
        {
            return nav(p);
        }
        
        //! Get a specific Bson object at a path.
        /*!
         \par 
         Syntatical sugar for \c nav(p).
         \sa nav(const std::string)const
         \param p The path to follow.
         \return Reference to the object at that node.
         \throw Exception if the node cannot be navigated to.
         */
        inline const Bson& operator[](const std::string& p) const
        {
            return nav(p);
        }
        
        //! Set a child at a specific path.
        /*!
         \par
         The parent Bson becomes responsibile for the destruction of the pointer \c child.
         \par
         If \c path is an empty string, \c set_child becomes a no-op.
         \par
         If \c child is null, the child specified by \c path is removed.
         \param path The path to set the child for.
         \param child The child to set.
         */
        void set_child(const std::string& path, Bson* child);
        
        //! Push a child at a specific path.
        /*!
         \par 
         The parent Bson becomes responsible for the destruction of the pointer \c child.
         \par
         If \c path is an empty string, the child is pushed onto the
         current Bson object.
         \par
         If \c child is null, \c push_child becomes a no-op.
         \param path The path where the child should be pushed.
         \param child The child to push.
         */
        void push_child(const std::string& path, Bson* child);

        //! Push a child onto this Bson object.
        Bson& operator<<(const Bson& o);
        
        //! Get the map backing document type.
        /*!
         \return A map of children. An empty map for non-document types.
         */
        inline const Linked_map<std::string, Bson*>& to_map() const
        {
            return child_map_;
        }
                
        //! Get the value of this object.
        /*!
         \return The value of this node. NULL for document and array types.
         */
        inline const char* to_value() const
        {
            return value_;
        }
        
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
        inline char* to_binary() const
        {
            char *ptr = new char[size()];
            copy_to_bson(ptr);
            return ptr;
        }

        //! Get the type of the document node.
        Bson::Type type() const
        {
            return type_;
        }
        
        //! Get if the Bson object contains anything.
        bool exists() const;
        
        //! Get the size of the node.
        size_t size() const;
        
    private:
        Linked_map<std::string, Bson*> child_map_;
        int last_child_;
        char* value_;
        Bson::Type type_;
        //! copy the value of this object into a bson byte array.
        size_t copy_to_bson(char *) const;
    };
    
    //! Get a string version of the type.
    /*!
     \param t The type to convert into a string.
     \return String name for the type.
     */
    const std::string& bson_type_string(const Bson::Type t);
    
    //! Get the minimum number of bytes required for a type.
    /*!
     \param t The type to get the required bytes for.
     \return The minimum size of the type.
     */
    size_t bson_type_min_size(const Bson::Type t);
    
    //! Test if a type is a nested type (Array, or document).
    /*!
     \param t The type to test.
     \return True if the type is a nested object, false otherwise.
     */
    inline bool bson_type_is_nested(const Bson::Type t)
    {
        return (t == Bson::k_document ||
                t == Bson::k_array);
    }
    
    //! Test if a type is quotablable (string types).
    /*!
     \param t The type to test.
     \return True if the type is a quotable object, false otherwise.
     */
    inline bool bson_type_is_quotable(const Bson::Type t)
    {
        return (t == Bson::k_string);
    }
    
    //! Test if a type is a numerical type (integers and floats).
    /*!
     \param t The type to test.
     \return True if the type is a number type, false otherwise.
     */
    inline bool bson_type_is_number(const Bson::Type t)
    {
        return (t == Bson::k_int32 ||
                t == Bson::k_int64 ||
                t == Bson::k_timestamp ||
                t == Bson::k_double);
    }
    
    //! Test if a type is a native c++ type (integers, floats, booleans, null).
    /*!
     \param t The type to test.
     \return True if the type is a native type, false otherwise.
     */
    inline bool bson_type_is_native(const Bson::Type t)
    {
        return (t == Bson::k_int32 ||
                t == Bson::k_int64 ||
                t == Bson::k_timestamp ||
                t == Bson::k_double ||
                t == Bson::k_boolean ||
                t == Bson::k_null);
    }
    
    //! Create a new Bson string object.
    /*!
     \par
     Object should be released with delete.
     \param str The string value.
     \return a new Bson object.
     */
    Bson* bson_new_string(const std::string& str);
    
    //! Create a new Bson boolean object.
    /*!
     \par
     Object should be released with delete.
     \param val The boolean value.
     \return a new Bson object.
     */
    Bson* bson_new_boolean(const bool val);

    //! Create a new int64 object.
    /*!
     \par
     Object should be released with delete.
     \param val The integer value.
     \return a new Bson object.
     */
    Bson* bson_new_int64(const int64_t val);

    //! Create a new null object.
    /*!
     \par
     Object should be released with delete.
     \return a new Bson object.
     */
    Bson* bson_new_null();
        
    //! Get the value of a Bson object as a C++ string in debug format.
    /*!
     \par
     The debug string is a representation of the Bson object in BSON
     format.  Rather than being a byte array, the results are output in
     a pseudo-JSON looking format, with lengths and byte counts included
     in the display.
     \par
     This is really only useful for debugging output.
     \param b The Bson object
     \return A string describing how this Bson object should look in BSON.
     */
    std::string bson_as_debug_string(const Bson& b);
    
    //! Get the value of a Bson object as a c++ string.
    /*!
     \par
     Value types are output in their string representation.  Document and
     array types are output in a JSON looking format.
     \param b The Bson object.
     \return A string describing how this Bson object should look in JSON.
     */
    std::string bson_as_string(const Bson& b);
    
    //! Get the value of a Bson object as a c++ string with indenting.
    /*!
     \par
     The pretty string is a representation of the Bson object in JSON
     format.  The nested structures are indented to make the structure
     easier to read.
     \par
     Value types are output in their string representation.  Document and
     array types are output in a JSON looking format.
     \param b The Bson object.
     \return A string describing how this Bson object should look in JSON with indentation.
     */
    std::string bson_as_pretty_string(const Bson& b, int lvl = 0);
    
    //! Get the set of keys.
    /*!
     \par
     Value and array types return an empty set. Document types return
     the set of keys.
     \param b The Bson object.
     \return Set of all keys for this object.
     */
    std::set<std::string> bson_as_key_set(const Bson& b);
    
    //! Get the set of values.
    /*!
     \par
     Valye types return a set containing themselves. Document and array types
     return a set containing all child values as strings (not pretty or
     debug strings).
     \param b The Bson object.
     \return A set of values.
     \sa bson_as_string(const Bson&)
     */
    std::set<std::string> bson_as_value_string_set(const Bson& b);
    
    int32_t bson_as_int32(const Bson& b);
    int64_t bson_as_int64(const Bson& b);
    bool bson_as_boolean(const Bson& b);
    double bson_as_double(const Bson& b);
    void bson_save(const Bson& b, const std::string& path);
    Bson* bson_load(const std::string& path);
}; // namespace lj
