#pragma once
/*!
 \file lj/Bson.h
 \brief LJ Bson header.

 Copyright (c) 2014, Jason Watson
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
        /*!
         \brief Enumeration of Bson Types.
         \since 1.0
         \sa http://bsonspec.org/ for the actual spec.
         */
        enum class Type : uint8_t
        {
            k_double = 0x01, //!< Node contains a double value.
            k_string = 0x02, //!< Node contains a string value.
            k_document = 0x03, //!< Node contains a nested document value.
            k_array = 0x04, //!< Node contains a nested array value.
            k_binary = 0x05, //!< Node contains a binary value.
            k_binary_document = 0x06, //!< Node contains a document that has not been parsed (Raw bytes).
            k_boolean = 0x08, //!< Node contains a boolean value.
            k_datetime = 0x09, //!< Node contains a date/time value.
            k_null = 0x0A, //!< Node contains a null value.
            k_javascript = 0x0D, //!< Node contains a javascript value.
            k_int32 = 0x10, //!< Node contains a int32 number value.
            k_timestamp = 0x11, //!< Node contains a timestamp value.
            k_int64 = 0x12, //!< Node contains a int64 number value.
            k_minkey = 0xFF, //!< Node contains a reserved BSON spec value.
            k_maxkey = 0x7F //!< Node contains a reserved BSON spec value.
        };

        /*!
         \brief Enumeration of Bson binary subtypes.
         \since 1.0
         \sa http://bsonspec.org/ for the actual spec.
         */
        enum class Binary_type : uint8_t
        {
            k_bin_generic = 0x00, //!< Generic Binary
            k_bin_function = 0x01, //!< Function.
            k_bin_binary = 0x02, //!< Old Binary String.
            k_bin_uuid = 0x03, //!< Old UUID setting.
            k_bin_md5 = 0x05, //!< MD5.
            k_bin_user_defined = 0x80 //!< User defined binary string.
        };

        //! Get a string version of the type.
        const std::string& type_string(const Type t);

        //! Get a string version of the binary type.
        const std::string& binary_type_string(const Binary_type subtype);

        //! Get the minimum number of bytes required for a type.
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
        inline bool type_is_nested(const Type t)
        {
            return (t == Type::k_document ||
                    t == Type::k_array);
        }

        //! Test if a type is a nested type for pretty printing (Array, UUID, or document).
        inline bool type_is_pretty_nested(const Type t)
        {
            return (type_is_nested(t) ||
                    t == Type::k_binary);
        }

        //! Test if a type is a value type (to_value() will succeed).
        inline bool type_is_value(const Type t)
        {
            return !type_is_nested(t);
        }

        //! Test if a type is quotablable (string types).
        inline bool type_is_quotable(const Type t)
        {
            return (t == Type::k_string);
        }

        //! Test if a type is a numerical type (integers and floats).
        inline bool type_is_number(const Type t)
        {
            return (t == Type::k_int32 ||
                    t == Type::k_int64 ||
                    t == Type::k_timestamp ||
                    t == Type::k_double);
        }

        //! Test if a type is a native c++ type (integers, floats, booleans, null).
        inline bool type_is_native(const Type t)
        {
            return (t == Type::k_int32 ||
                    t == Type::k_int64 ||
                    t == Type::k_timestamp ||
                    t == Type::k_double ||
                    t == Type::k_boolean ||
                    t == Type::k_null);
        }

        /*!
         \brief Bson path exception
         \since 1.0

         Represents an invalid path in a bson document.
         */
        class Bson_path_exception : public lj::Exception
        {
        public:
            //! Constructor.
            Bson_path_exception(const std::string& msg, const std::string path) :
                    lj::Exception("Bson", msg),
                    path_(path)
            {
            }

            //! Copy constructor.
            Bson_path_exception(const Bson_path_exception& o) :
                    lj::Exception(o),
                    path_(o.path_)
            {
            }

            //! Move constructor.
            Bson_path_exception(Bson_path_exception&& o) :
                    lj::Exception(o),
                    path_(std::move(o.path_))
            {
            }

            //! Destructor.
            virtual ~Bson_path_exception() throw ()
            {
            }

            //! Copy assignment operator
            Bson_path_exception& operator=(const Bson_path_exception& o)
            {
                if (&o != this)
                {
                    lj::Exception::operator=(o);
                    path_ = o.path_;
                }
                return *this;
            }

            //! Move assignment operator
            Bson_path_exception& operator=(Bson_path_exception&& o)
            {
                if (&o != this)
                {
                    lj::Exception::operator=(o);
                    path_ = std::move(o.path_);
                }
                return *this;
            }

            //! Get the path that caused this exception.
            virtual std::string path() const
            {
                return path_;
            }

            virtual std::string str() const override;
        private:
            std::string path_;
        };

        /*!
         \brief Bson type exception.
         \since 1.0

         Represents an invalid type operation in a bson document.
         */
        class Bson_type_exception : public lj::Exception
        {
        public:
            //! Constructor.
            Bson_type_exception(const std::string& msg,
                    Type type,
                    Binary_type binary_type = Binary_type::k_bin_generic) :
                    lj::Exception("Bson", msg),
                    type_(type),
                    binary_type_(binary_type)
            {
            }

            //! Copy constructor.
            Bson_type_exception(const Bson_type_exception& o) :
                    lj::Exception(o),
                    type_(o.type_),
                    binary_type_(o.binary_type_)
            {
            }

            //! Move constructor
            Bson_type_exception(Bson_type_exception&& o) :
                    lj::Exception(o),
                    type_(std::move(o.type_)),
                    binary_type_(std::move(o.binary_type_))
            {
            }

            //! Destructor
            virtual ~Bson_type_exception() throw ()
            {
            }

            //! Copy assignment operator
            Bson_type_exception& operator=(const Bson_type_exception& o)
            {
                if (&o != this)
                {
                    lj::Exception::operator=(o);
                    type_ = o.type_;
                    binary_type_ = o.binary_type_;
                }
                return *this;
            }

            //! Move assignment operator
            Bson_type_exception& operator=(Bson_type_exception&& o)
            {
                if (&o != this)
                {
                    lj::Exception::operator=(o);
                    type_ = std::move(o.type_);
                    binary_type_ = std::move(o.binary_type_);
                }
                return *this;
            }

            //! The type of the object.
            virtual Type type() const
            {
                return type_;
            }

            //! The binary type of the binary object.
            virtual Binary_type binary_type() const
            {
                return binary_type_;
            }

            virtual std::string str() const override;
        private:
            Type type_;
            Binary_type binary_type_;
        };

        /*!
         \brief Bson value.
         \since 1.0

         Represets a Bson value, including documents and arrays.
         */
        class Node
        {
        public:
            /*!
             \brief Create a new document Node.

             Creates a blank, empty node of Type::k_document
             */
            Node();

            /*!
             \brief Create a new document node based on some data.

             Create a new Node value based on the provided values. Used to create
             value nodes.

             The value of v is copied out of the pointer \c v. For document
             and array types, \c v may be NULL. For value types, \c v must be
             a valid pointer.

             \param t The type of node being created.
             \param v The value to associate with this node.
             \sa Node::set_value()
             */
            Node(const Type t, const uint8_t* v);

            //! Create a new document node as a copy of an existing Node.
            Node(const Node& o);

            //! Create a new document node as a move of an existing Node.
            Node(Node&& o);

            //! Destructor.
            ~Node();

            /*!
             \brief Set the value of the document node based on a bson string.

             The value of v is copied out of the pointer \c v. For document
             and array types, \c v may be NULL. For value types, \c v must be
             a valid pointer.

             \param t The new type of the document.
             \param v Array of data to read the new value from.
             */
            void set_value(const Type t, const uint8_t* v);

            /*!
             \brief Set the value of the document node to null.

             Nullified nodes exist, and contain the value null.
             */
            void nullify();

            //! Destroy the current value and copy values from another Node.
            Node& copy_from(const Node& o);

            /*!
             \brief Destroy the current value and copy values from another Node.
             \sa Node::copy_from()
             */
            inline Node& operator=(const Node& o)
            {
                if (&o != this)
                {
                    copy_from(o);
                }
                return *this;
            }

            //! Destroy the current value and move values from another node.
            Node& operator=(Node&& o);

            /*!
             \brief Navigate to a specific path, creating nodes that do not exist.

             A string consisting of the array index is allowed as part of the
             string list, but it will not automatically create array nodes or
             the items inside the array.

             \param a_path A list of path elements to navigate through.
             \return Pointer to the found or created object.
             */
            Node* find_or_create_child_documents(const std::list<std::string>& a_path);

            /*!
             \brief Get a pointer to a specific Node object in the document.

             Similar to \c #find_or_create_child_documents(const std::list<std::string>&)
             except that it takes a string with path elements separated with
             forward slashes.
             \param p The path to follow.
             \return Pointer to the found or created object.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             \sa #nav(const std::string&)
             */
            Node* path(const std::string& p);

            /*!
             \brief Get a pointer to a specific Node object in the document.

             Similar to \c #find_or_create_child_documents(const std::list<std::string>&)
             except that it takes a string with path elements separated with
             forward slashes. It also skips the "create" part. If the path
             cannot be found, this function returns nullptr.
             \param p The path to follow.
             \return Pointer to the found object or \c nullptr
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             \sa #nav(const std::string&)const
             */
            const Node* path(const std::string& p) const;

            /*!
             \brief Get a specific Node object at a path.

             Reference version of \c #path(const std::string&).
             \param p The path to follow.
             \return Reference to the object at that node.
             \sa path(const std::string&)
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            inline Node& nav(const std::string& p)
            {
                return *path(p);
            }

            /*!
             \brief Get a specific Node object at a path.

             Reference version of \c #path(const std::string&)const.
             \sa #path(const std::string&)const
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

            /*!
             \brief Get a specific Node object at a path.

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

            /*!
             \brief Get a specific Node object at a path.
             
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

            /*!
             \brief Set a child at a specific path.

             The parent Node assumes responsibility for the destruction of
             the pointer \c child.

             If \c path is an empty string, \c set_child becomes a no-op.

             If \c child is null, the child specified by \c path is removed.
             \param path The path to set the child for.
             \param child The child to set.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            void set_child(const std::string& path, Node* child);

            /*!
             \brief Push a child at a specific path.

             The parent Node becomes responsible for the destruction of the
             pointer \c child.

             If \c path is an empty string, the child is pushed onto the
             current Node object.

             If \c child is null, \c push_child becomes a no-op.
             \param path The path where the child should be pushed.
             \param child The child to push.
             \sa #find_or_create_child_documents(const std::list<std::string>&)
             */
            void push_child(const std::string& path, Node* child);

            /*!
             \brief Push a child onto this Node object.
             \param o other object to copy from.
             \return reference to this.
             */
            Node& operator<<(const Node& o)
            {
                push_child("", new Node(o));
                return *this;
            }

            /*!
             \brief Push a child onto this Node object.

             The parent Node becomes responsible for the destruction of the
             pointer \c child.
             \param o other object pointer to attach
             \return reference to this.
             */
            inline Node& operator<<(Node* o)
            {
                push_child("", o);
                return *this;
            }

            /*!
             \brief Get the map backing document nodes.
             \return A map of children.
             \throws lj::bson::Bson_type_exception When called on
             non-document nodes.
             */
            inline const std::map<std::string, Node*>& to_map() const
            {
                if (Type::k_document != type())
                {
                    throw Bson_type_exception("Unable to represent object as a map.", type());
                }
                return *(value_.map_);
            }

            /*!
             \brief Get the vector backing array type.
             \return A vector of children.
             \throws lj::bson::Bson_type_exception When called on
             non-array types.
             */
            inline const std::vector<Node*>& to_vector() const
            {
                if (Type::k_array != type())
                {
                    throw Bson_type_exception("Unable to represent object as a vector.", type());
                }
                return *(value_.vector_);
            }

            /*!
             \brief Get the value of this object.
             \return The value of this node.
             \throws lj::bson::Bson_type_exception for document and array types.
             */
            inline const uint8_t* to_value() const
            {
                if (type_is_nested(type()))
                {
                    throw Bson_type_exception("Unable to represent object as a data pointer.", type());
                }
                return value_.data_;
            }

            /*!
             \brief get the value of the document node as a bson string.

             Pointer is allocated with \c new[] and must be released with
             \c delete[].

             The array length can be obtained by calling \c size(), but it is
             recommended that you use the size pointer to get the size.
             \param sz_ptr [out] Location to store the size of the data.
             \return A byte array contain the bson document.
             */
            inline uint8_t* to_binary(size_t* sz_ptr) const
            {
                size_t sz = size();
                uint8_t *ptr = new uint8_t[sz];
                copy_to_bson(ptr);

                // if sz_ptr is not null, store the data size.
                if (sz_ptr)
                {
                    *sz_ptr = sz;
                }
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

            /*! Get the size of the node.

             This requires traversing the entire node tree and can
             be expensive on larger documents.
             */
            size_t size() const;

        private:
            Type type_;

            union
            {
                uint8_t* data_;
                std::vector<Node*>* vector_;
                std::map<std::string, Node*>* map_;
            } value_;

            size_t copy_to_bson(uint8_t *) const;

            void destroy(bool);
        }; // class lj::bson::Node

        /*!
         \brief Escape slashes for bson keys.
         \param input The string to escape.
         \return The escaped string.
         */
        std::string escape_path(const std::string& input);

        /*!
         \brief Create a new string object.

         Pointer should be released with delete.
         \param str The string value.
         \return a new Node object.
         */
        Node* new_string(const std::string& str);

        /*!
         \brief Create a new boolean object.

         Pointer should be released with delete.
         \param val The boolean value.
         \return a new Node object.
         */
        Node* new_boolean(const bool val);

        /*!
         \brief Create a new int32 object.

         Pointer should be released with delete.
         \param val The integer value.
         \return a new Node object.
         */
        Node* new_int32(const int32_t val);

        /*!
         \brief Create a new int64 object.

         Pointer should be released with delete.
         \param val The integer value.
         \return a new Node object.
         */
        Node* new_int64(const int64_t val);

        /*!
         \brief Create a new int64 object.

         Pointer should be released with delete.
         \param val The integer value.
         \return a new Node object.
         */
        Node* new_uint64(const uint64_t val);

        /*!
         \brief Create a new null object.

         Pointer should be released with delete.
         \return a new Node object.
         */
        Node* new_null();

        /*!
         \brief Create a new binary object.

         Pointer should be released with delete.
         \param val The binary value
         \param sz The size of the binary value.
         \param subtype The binary subtype.
         \return a new Node object.
         */
        Node* new_binary(const uint8_t* val, uint32_t sz, Binary_type subtype);

        /*!
         \brief Create a new Uuid Bson object.

         A Uuid Node object is a binary type that contains 16 bytes.

         Pointer should be released with delete.
         \param uuid The unique ID.
         \return a new Node object.
         */
        Node* new_uuid(const lj::Uuid& uuid);

        /*!
         \brief Create a new array Bson object.

         An array bson object. May contain child nodes.

         Pointer should be released with delete.
         \return a new Node object.
         */
        Node* new_array();

        /*!
         \brief Create a new node from a json string.

         The input string is expected to be a well-formed json document
         that will be parsed into a bson document node.

         Pointer should be released with delete.
         \param val The string value to parse.
         \return A new Node object.
         \exception lj::Exception Upon encountering unparsable data in the
         string.
         */
        Node* parse_json(const std::string& val);

        /*!
         \brief Create a new node from a json std::istream.

         The input stream is expected to be a well-formed json document
         that will be parsed into a bson document node.

         Pointer should be released with delete.
         \param in_stream The input stream.
         \return A new Node object.
         \exception lj::Exception Upon encountering unparsable data in the
         input stream.
         */
        Node* parse_json(std::istream& in_stream);

        /*!
         \brief Get the value of a Bson object as a C++ string in debug format.

         The debug string is a representation of the Node object in BSON
         format.  Rather than being a byte array, the results are output in
         a pseudo-JSON looking format, with lengths and byte counts included
         in the display.

         This is really only useful for debugging bson output.
         \param b The Node object
         \param lvl The indentation level.
         \return A string describing how this Node object should look in BSON.
         */
        std::string as_debug_string(const Node& b, int lvl = 1);

        /*!
         \brief Get the value of a Bson object as a c++ string.

         Value types are output in their string representation.  Document and
         array types are output in a JSON looking format.
         \param b The Node object.
         \return A string describing how this Node object should look in JSON.
         */
        std::string as_string(const Node& b);

        /*!
         \brief Get the value of a Bson object as a json string with indenting.

         The pretty string is a representation of the Node object in JSON
         format.  The nested structures are indented to make the structure
         easier to read.

         The result can be converted back into a BSON object using the
         json string parsing method.

         Value types are output in their string representation.  Document and
         array types are output in a JSON looking format.
         \param b The Node object.
         \param lvl The current indention level.
         \return A string describing how this Node object should look in JSON with indentation.
         */
        std::string as_json_string(const Node& b, int lvl = 1);

        /*!
         \brief Get the value of a Bson object as an 32-bit wide integer.

         Value types are converted into numbers. Document, Array, and strings that
         cannot be converted into a number return 0.
         \param b The Node object.
         \return The number value.
         */
        int32_t as_int32(const Node& b);

        /*!
         \brief Get the value of a Bson object as an 64-bit wide integer.

         Value types are converted into numbers. Document, Array, and strings that
         cannot be converted into a number return 0.
         \param b The Node object.
         \return The number value.
         */
        int64_t as_int64(const Node& b);

        /*!
         \brief Get the value of a Bson object as an unsigned 64-bit wide integer.

         Value types are converted into numbers. Document, Array and strings that
         cannot be converted into a number return 0.
         \param b The Node object.
         \return The number value.
         */
        uint64_t as_uint64(const Node& b);

        /*!
         \brief Get the value of a Bson object as a boolean.

         If the value is a string, return true only if the string contains the
         word "true" or the character '1'.

         Numeric types return true if the value is not equal to 0.
         
         Document, Null, and Array types always return false.
         \param b The Node object.
         \return The boolean value.
         */
        bool as_boolean(const Node& b);

        /*!
         \brief Get the value of a bson object as a double.

         Value types are converted into numbers. Document, Array, and string that
         cannot be converted into a number return 0.0.
         \param b The Node object.
         \return The double value.
         */
        double as_double(const Node& b);

        /*!
         \brief Get the value of a bson object as a pointer.

         Only works if the Node is a binary type.

         Returned value points to an internal data structure and should not be
         freed. Pointer will become invalid if the Node object associated with
         the value is deleted.
         \param b The Node object.
         \param t Location to store the subtype in.
         \param sz Location to store the length in.
         \return pointer to a string of bytes, NULL otherwise.
         */
        const uint8_t* as_binary(const Node& b, Binary_type* t, uint32_t* sz);

        /*!
         \brief Get the value of a bson object as a lj::Uuid.

         Only works if the Node is a binary type with the subtype of uuid.

         Returned value is independent from the Node object and is still
         valid after the Node object is deleted.
         \param b The Node object.
         \return The Uuid.
         */
        Uuid as_uuid(const Node& b);

        /*!
         \brief Increment the value of a bson object.

         Converts a value to an integer if it is not already.
         \param b The Node object to modify.
         \param amount The amount to increment a value. May be negative.
         */
        void increment(Node& b, int amount);

        /*!
         \brief Combine data in changes to the target node.

         Adds all of the paths that exist in \c changes to \c target. Arrays
         and other values in the document are added. If a value already exists
         at a path, it is replaced by the changes value.
         \param target [in,out] The target \c Node to modify.
         \param changes [in] The paths and data to add to \c target.
         */
        void combine(Node& target, const Node& changes);
    }; // namespace lj::bson
}; // namespace lj

/*!
 \brief Extract data with format.

 Extract an lj::bson::Node object from the datastream. The object will be
 marshaled from the binary representation.
 \param is The input stream to read from.
 \param val The Node to store the data in.
 \return The input stream passed as \c is.
 */
std::istream& operator>>(std::istream& is, lj::bson::Node& val);

/*!
 \brief Insert data with format.

 Insert an lj::bson::Node object to the datastream. The node will be converted
 to binary form and written out.
 \param os The output stream to write to.
 \param val The Node to copy to binary.
 \return The output stream passed as \c os.
 */
std::ostream& operator<<(std::ostream& os, const lj::bson::Node& val);
