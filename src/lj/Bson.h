#pragma once
/*!
 \file lj/Bson.h
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
#include "lj/Uuid.h"

#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace lj
{
    namespace bson
    {
        //! Enumeration of Bson Types.
        enum class Type : uint8_t
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

        //! Enumeration of Bson binary subtypes.
        enum class Binary_type : uint8_t
        {
            k_bin_generic = 0x00,      //!< Generic Binary
            k_bin_function = 0x01,     //!< Function.
            k_bin_binary = 0x02,       //!< Binary String.
            k_bin_uuid = 0x03,         //!< UUID.
            k_bin_md5 = 0x05,          //!< MD5.
            k_bin_user_defined = 0x80  //!< User defined binary string.
        };
    
        //! Get a string version of the type.
        /*!
         \param t The type to convert into a string.
         \return String name for the type.
         */
        const std::string& type_string(const Type t);

        //! Get a string version of the binary type.
        /*!
         \param subtype The type to convert into a string.
         \return String name for the type.
         */
        const std::string& binary_type_string(const Binary_type subtype);

        //! Get the minimum number of bytes required for a type.
        /*!
         \param t The type to get the required bytes for.
         \return The minimum size of the type.
         */
        inline size_t type_min_size(const Type t)
        {
            switch (t)
            {
                case Type::k_null:
                    return 0;
                case Type::k_boolean:
                    return 1;
                case Type::k_int32:
                    return 4;
                case Type::k_string:
                case Type::k_binary:
                case Type::k_binary_document:
                case Type::k_document:
                case Type::k_array:
                    return 5;
                case Type::k_timestamp:
                case Type::k_int64:
                case Type::k_double:
                    return 8;
                default:
                    break;
            }
            return 5;
        }
        
        //! Test if a type is a nested type (Array, or document).
        /*!
         \param t The type to test.
         \return True if the type is a nested object, false otherwise.
         */
        inline bool type_is_nested(const Type t)
        {
            return (t == Type::k_document ||
                    t == Type::k_array);
        }
        
        //! Test if a type is a value type (to_value() will succeed).
        /*!
         \param t the type to test.
         \return True if the type supports to_valeu(), false otherwise.
         */
        inline bool type_is_value(const Type t)
        {
            return !type_is_nested(t);
        }

        //! Test if a type is quotablable (string types).
        /*!
         \param t The type to test.
         \return True if the type is a quotable object, false otherwise.
         */
        inline bool type_is_quotable(const Type t)
        {
            return (t == Type::k_string);
        }

        //! Test if a type is a numerical type (integers and floats).
        /*!
         \param t The type to test.
         \return True if the type is a number type, false otherwise.
         */
        inline bool type_is_number(const Type t)
        {
            return (t == Type::k_int32 ||
                    t == Type::k_int64 ||
                    t == Type::k_timestamp ||
                    t == Type::k_double);
        }

        //! Test if a type is a native c++ type (integers, floats, booleans, null).
        /*!
         \param t The type to test.
         \return True if the type is a native type, false otherwise.
         */
        inline bool type_is_native(const Type t)
        {
            return (t == Type::k_int32 ||
                    t == Type::k_int64 ||
                    t == Type::k_timestamp ||
                    t == Type::k_double ||
                    t == Type::k_boolean ||
                    t == Type::k_null);
        }
        
        //! Bson path exception
        /*!
         \par
         Represents an invalid path in a bson document.
         \author Jason Watson
         \version 1.0
         \date May 22, 2011
         */
        class Bson_path_exception : public lj::Exception
        {
        public:
            //! Constructor.
            /*!
             \param msg The exception message.
             \param path The path that caused the exception.
             */
            Bson_path_exception(const std::string& msg, const std::string path) : lj::Exception("Bson", msg), path_(path)
            {
            }

            //! Copy constructor.
            /*!
             \param o The other object.
             */
            Bson_path_exception(const Bson_path_exception& o) : lj::Exception(o.label_, o.msg_)
            {
            }
            
            //! Destructor.
            virtual ~Bson_path_exception() throw()
            {
            }

            //! Get the path that caused this exception.
            /*!
             \return The path.
             */
            virtual std::string path() const
            {
                return path_;
            }
            
            virtual std::string str() const;
        private:
            std::string path_;
        };
        
        //! Bson type exception.
        /*!
         \par
         Represents an invalid type operation in a bson document.
         \author Jason Watson
         \version 1.0
         \date May 22, 2011
         */
        class Bson_type_exception : public lj::Exception
        {
        public:
            //! Constructor.
            /*!
             \param msg The exception msg.
             \param type The bson type of the object.
             \param binary_type The binary sub-type for binary objects.
             */
            Bson_type_exception(const std::string& msg, Type type, Binary_type binary_type = Binary_type::k_bin_generic) : lj::Exception("Bson", msg), type_(type), binary_type_(binary_type)
            {
            }
            
            //! Copy constructor.
            /*!
             \param o The other object.
             */
            Bson_type_exception(const Bson_type_exception& o) : lj::Exception(o.label_, o.msg_)
            {
            }
            
            //! Destructor
            virtual ~Bson_type_exception() throw()
            {
            }

            //! The type of the object.
            /*!
             \return The type.
             */
            virtual Type type() const
            {
                return type_;
            }
            
            //! The binary type of the binary object.
            /*!
             \par
             Only useful when the type is "binary".
             \return The binary sub-type.
             */
            virtual Binary_type binary_type() const
            {
                return binary_type_;
            }
            
            virtual std::string str() const;
        private:
            Type type_;
            Binary_type binary_type_;
        };

        //! Bson value.
        /*!
         \par
         Represets a Bson value, including documents and arrays. The following
         examples show different ways to set a value at some path.
         \author Jason Watson
         \version 1.0
         \date April 19, 2010
         */
        class Node
        {
        public:
            //! Create a new document Node.
            /*!
             \par
             Creates a blank, empty document that considers itself non-existant.
             Used to create document nodes.
             */
            Node();

            //! Create a new document node based on some data.
            /*!
             \par
             Create a new Node value based on the provided values. Used to create
             value nodes.
             \par
             Data from \c v is copied to internal objects.
             \param t The type of node being created.
             \param v The value to associate with this node.
             \sa Node::set_value()
             */
            Node(const Type t, const uint8_t* v);

            //! Create a new document node as a copy of an existing Node.
            /*!
             \param o The original Node object.
             \sa Node::copy_from()
             */
            Node(const Node &o);

            //! Destructor.
            ~Node();

            //! Set the value of the document node based on a bson string.
            /*!
             \par
             The value of v is copied out of the pointer \c v , and must be freed
             by the calling application.
             \param t The new type of the document.
             \param v Array of data to read the new value from.
             */
            void set_value(const Type t, const uint8_t* v);

            //! Set the value of the document node to null.
            /*!
             \par
             Nullified nodes exist, and contain the value null.
             */
            void nullify();

            //! Destroy the current value and copy values from another Node.
            /*!
             \param o This node object.
             */
            Node& copy_from(const Node& o);

            //! Destroy the current value and copy values from another Node.
            /*!
             \par
             \c a \c = \c b is identical to calling \c a.copy_from(b).
             \param o The original Node object.
             \return Reference to this object.
             \sa Node::copy_from()
             */
            inline Node& operator=(const Node& o)
            {
                copy_from(o);
                return *this;
            }

            //! Takes a list of strings, and creates child documents when they do not exist.
            Node* find_or_create_child_documents(const std::list<std::string>&);
            
            //! Get a pointer to a specific Node object in the document.
            /*!
             \par
             The intermediate objects in the path are created if they do not exist.
             \param p The path to follow.
             \return Pointer to that object.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            Node* path(const std::string& p);

            //! Get a pointer to a specific Node object in the document.
            /*!
             \par
             Null is returned if the intermediate objects in the path do not exist.
             \param p The path to follow.
             \return Pointer to that object, null if it cannot be navigated to.
             */
            const Node* path(const std::string& p) const;

            //! Get a specific Node object at a path.
            /*!
             \par 
             Reference version of \c path(p).
             \sa path(const std::string)
             \param p The path to follow.
             \return Reference to the object at that node.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            inline Node& nav(const std::string& p)
            {
                return *path(p);
            }

            //! Get a specific Node object at a path.
            /*!
             \par 
             Reference version of \c path(p).
             \sa path(const std::string)const
             \param p The path to follow.
             \return Reference to the object at that node.
             \throw Exception if the node cannot be navigated to.
             */
            inline const Node& nav(const std::string& p) const
            {
                const Node* ptr = path(p);
                if (!ptr)
                {
                    throw Bson_path_exception("Path not found.", p);
                }
                return *ptr;
            }

            //! Get a specific Node object at a path.
            /*!
             \par 
             Syntatical sugar for \c nav(p).
             \sa nav(const std::string)
             \param p The path to follow.
             \return Reference to the object at that node.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            inline Node& operator[](const std::string& p)
            {
                return nav(p);
            }

            //! Get a specific Node object at a path.
            /*!
             \par 
             Syntatical sugar for \c nav(p).
             \sa nav(const std::string)const
             \param p The path to follow.
             \return Reference to the object at that node.
             \throw Exception if the node cannot be navigated to.
             */
            inline const Node& operator[](const std::string& p) const
            {
                return nav(p);
            }

            //! Set a child at a specific path.
            /*!
             \par
             The parent Node becomes responsibile for the destruction of the pointer \c child.
             \par
             If \c path is an empty string, \c set_child becomes a no-op.
             \par
             If \c child is null, the child specified by \c path is removed.
             \param path The path to set the child for.
             \param child The child to set.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            void set_child(const std::string& path, Node* child);

            //! Push a child at a specific path.
            /*!
             \par 
             The parent Node becomes responsible for the destruction of the pointer \c child.
             \par
             If \c path is an empty string, the child is pushed onto the
             current Node object.
             \par
             If \c child is null, \c push_child becomes a no-op.
             \param path The path where the child should be pushed.
             \param child The child to push.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            void push_child(const std::string& path, Node* child);

            //! Push a child onto this Node object.
            /*!
             \param o other object to copy from.
             \return reference to this.
             */
            Node& operator<<(const Node& o)
            {
                push_child("", new Node(o));
                return *this;
            }

            //! Push a child onto this Node object.
            /*!
             \note Memory
             This object becomes responsible for the memory.
             \param o other object pointer to attach
             \return reference to this.
             */
            inline Node& operator<<(Node* o)
            {
                push_child("", o);
                return *this;
            }

            //! Get the map backing document type.
            /*!
             \return A map of children. An empty map for non-document types.
             */
            inline const std::map<std::string, Node*>& to_map() const
            {
                if (Type::k_document != type())
                {
                    throw Bson_type_exception("Unable to represent object as a map.", type());
                }
                return *(value_.map_);
            }

            //! Get the vector backing array type.
            inline const std::vector<Node*>& to_vector() const
            {
                if (Type::k_array != type())
                {
                    throw Bson_type_exception("Unable to represent object as a vector.", type());
                }
                return *(value_.vector_);
            }

            //! Get the value of this object.
            /*!
             \return The value of this node. NULL for document and array types.
             */
            inline const uint8_t* to_value() const
            {
                if (type_is_nested(type()))
                {
                    throw Bson_type_exception("Unable to represent object as a data pointer.", type());
                }
                return value_.data_;
            }

            //! get the value of the document node as a bson string.
            /*!
             \par
             Pointer is allocated with "new[]" and must be released with "delete[]".
             \par
             The array length can be obtained by calling \c size() .
             \return A byte array contain the bson document.
             */
            inline uint8_t* to_binary() const
            {
                uint8_t *ptr = new uint8_t[size()];
                copy_to_bson(ptr);
                return ptr;
            }

            //! Get the type of the document node.
            inline Type type() const
            {
                return type_;
            }

            //! Get if the Node object contains anything.
            bool exists(const std::string& path) const
            {
                return (this->path(path) != NULL);
            }

            //! Get the size of the node.
            size_t size() const;

        private:
            Type type_;
            union {
                uint8_t* data_;
                std::vector<Node*>* vector_;
                std::map<std::string, Node*>* map_;
            } value_;
            
            //! copy the value of this object into a bson byte array.
            size_t copy_to_bson(uint8_t *) const;

            //! Set the value of the document node to not exist.
            void destroy(bool);
        }; // class Node

        //! Escape slashes for bson keys.
        /*!
         \param input The string to escape.
         \return The escaped string.
         */
        std::string escape_path(const std::string& input);

        //! Create a new string object.
        /*!
         \par
         Object should be released with delete.
         \param str The string value.
         \return a new Node object.
         */
        Node* new_string(const std::string& str);

        //! Create a new boolean object.
        /*!
         \par
         Object should be released with delete.
         \param val The boolean value.
         \return a new Node object.
         */
        Node* new_boolean(const bool val);
        
        //! Create a new int32 object.
        /*!
         \par
         Object should be released with delete.
         \param val The integer value.
         \return a new Node object.
         */
        Node* new_int32(const int32_t val);

        //! Create a new int64 object.
        /*!
         \par
         Object should be released with delete.
         \param val The integer value.
         \return a new Node object.
         */
        Node* new_int64(const int64_t val);

        //! Create a new int64 object.
        /*!
         \par
         Object should be released with delete.
         \param val The integer value.
         \return a new Node object.
         */
        Node* new_uint64(const uint64_t val);

        //! Create a new null object.
        /*!
         \par
         Object should be released with delete.
         \return a new Node object.
         */
        Node* new_null();

        //! Create a new binary object.
        /*!
         \par
         Object should be released with delete.
         \param val The binary value
         \param sz The size of the binary value.
         \param subtype The binary subtype.
         \return a new Node object.
         */
        Node* new_binary(const uint8_t* val, uint32_t sz, Binary_type subtype);

        //! Create a new Uuid Bson object.
        /*!
         \par
         A Uuid Node object is a binary type that contains 16 bytes.
         \par
         Result should be released with delete.
         \param uuid The unique ID.
         \return a new Node object.
         */
        Node* new_uuid(const lj::Uuid& uuid);

        //! Get the value of a Bson object as a C++ string in debug format.
        /*!
         \par
         The debug string is a representation of the Node object in BSON
         format.  Rather than being a byte array, the results are output in
         a pseudo-JSON looking format, with lengths and byte counts included
         in the display.
         \par
         This is really only useful for debugging output.
         \param b The Node object
         \param lvl The indentation level.
         \return A string describing how this Node object should look in BSON.
         */
        std::string as_debug_string(const Node& b, int lvl = 1);

        //! Get the value of a Bson object as a c++ string.
        /*!
         \par
         Value types are output in their string representation.  Document and
         array types are output in a JSON looking format.
         \param b The Node object.
         \return A string describing how this Node object should look in JSON.
         */
        std::string as_string(const Node& b);

        //! Get the value of a Bson object as a c++ string with indenting.
        /*!
         \par
         The pretty string is a representation of the Node object in JSON
         format.  The nested structures are indented to make the structure
         easier to read.
         \par
         Value types are output in their string representation.  Document and
         array types are output in a JSON looking format.
         \param b The Node object.
         \param lvl The current indention level.
         \return A string describing how this Node object should look in JSON with indentation.
         */
        std::string as_pretty_json(const Node& b, int lvl = 1);

        //! Get the value of a Bson object as an 32-bit wide integer.
        /*!
         \par
         Value types are converted into numbers. Document, Array, and strings that
         cannot be converted into a number return 0.
         \param b The Node object.
         \return The number value.
         */
        int32_t as_int32(const Node& b);

        //! Get the value of a Bson object as an 64-bit wide integer.
        /*!
         \par
         Value types are converted into numbers. Document, Array, and strings that
         cannot be converted into a number return 0.
         \param b The Node object.
         \return The number value.
         */
        int64_t as_int64(const Node& b);

        //! Get the value of a Bson object as an unsigned 64-bit wide integer.
        /*!
         \par
         Value types are converted into numbers. Document, Array and strings that
         cannot be converted into a number return 0.
         \param b The Node object.
         \return The number value.
         */
        uint64_t as_uint64(const Node& b);

        //! Get the value of a Bson object as a boolean.
        /*!
         \par
         If the value is a string, return true only if the string contains the
         word "true" or the character '1'.
         \par
         Numeric types return true if the value is not equal to 0.
         \par Document, Null, and Array types always return false.
         \param b The Node object.
         \return The boolean value.
         */
        bool as_boolean(const Node& b);

        //! Get the value of a bson object as a double.
        /*!
         \par
         Value types are converted into numbers. Document, Array, and string that
         cannot be converted into a number return 0.0.
         \param b The Node object.
         \return The double value.
         */
        double as_double(const Node& b);

        //! Get the value of a bson object as a pointer.
        /*!
         \par
         Only works if the Node is a binary type.
         \par
         Returned value points to an internal data structure and should not be
         freed. Pointer will become invalid if the Node object associated with
         the value is deleted.
         \param b The Node object.
         \param t Location to store the subtype in.
         \param sz Location to store the length in.
         \return pointer to a string of bytes, NULL otherwise.
         */
        const uint8_t* as_binary(const Node& b, Binary_type* t, uint32_t* sz);

        //! Get the value of a bson object as a lj::Uuid.
        /*!
         \par
         Only works if the Node is a binary type with the subtype of uuid.
         \par
         Returned value is independent from the Node object and is still
         valid after the Node object is deleted.
         \param b The Node object.
         \return The Uuid.
         */
        Uuid as_uuid(const Node& b);

        //! Increment the value of a bson object.
        /*!
         \par
         converts a value to an integer if it is not already.
         \param b The Node object to modify.
         \param amount The amount to increment a value. May be negative.
         */
        void increment(Node& b, int amount);
    }; // namespace lj::bson
}; // namespace lj

std::istream& operator>>(std::istream& is, lj::bson::Node& val);
std::ostream& operator<<(std::ostream& os, lj::bson::Node& val);
