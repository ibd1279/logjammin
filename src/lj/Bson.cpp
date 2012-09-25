/*!
 \file lj/Bson.cpp
 \brief LJ Bson implementation.
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

#include "lj/Bson.h"
#include "lj/Base64.h"
#include "lj/Log.h"
#include "Wiper.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <istream>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <stack>

namespace lj
{
    namespace bson
    {
        namespace
        {
            //! escape a string.
            std::string escape(const std::string& val)
            {
                std::string r;
                for (auto iter = val.begin(); iter != val.end(); ++iter)
                {
                    char c = *iter;
                    if (c == '\\' || c == '"')
                    {
                        r.push_back('\\');
                    }
                    r.push_back(c);
                }
                return r;
            }

            //! sub document parsing helper method.
            void subdocument(Type parent_t, Node& node, const uint8_t* value)
            {
                // calculate the end address, and position pointer after size.
                const uint8_t *ptr = value;
                long sz = 0;
                memcpy(&sz, ptr, 4);
                const uint8_t *end = ptr + sz - 1;
                ptr += 4;

                // if the size is 5, then this is the smallest sub document possible
                // and we don't need to process anything.
                if (sz == 5)
                {
                    return;
                }

                // loop while the pointer is before the end.
                while (ptr < end)
                {
                    // read the field type and advance the pointer.
                    uint8_t t;
                    memcpy(&t, ptr++, 1);

                    // Read the field name. BSON spec says field names are null
                    // terminated c-strings, so we let the std::string library
                    // find the hard work
                    std::string name(reinterpret_cast<const char*>(ptr));
                    ptr += name.size() + 1;

                    // Read the field value. this will create a child node.
                    // The node constructor does all the byte parsing based on
                    // the provided type.
                    Node* new_child = new Node(static_cast<Type>(t), ptr);

                    // Attaching the child node to the parent Node is different
                    // depending on the type of the parent node.
                    if (Type::k_document == parent_t)
                    {
                        node.set_child(escape_path(name), new_child);
                    }
                    else if(Type::k_array == parent_t)
                    {
                        node.push_child("", new_child);
                    }

                    // Move the pointer to the end of the parsed binary data.
                    // will put us at the start of the next field or the end of
                    // the document.
                    ptr += new_child->size();
                }
            }

            const std::string k_bson_type_string_string("string");
            const std::string k_bson_type_string_int32("int32");
            const std::string k_bson_type_string_double("double");
            const std::string k_bson_type_string_int64("int64");
            const std::string k_bson_type_string_timestamp("timestamp");
            const std::string k_bson_type_string_boolean("boolean");
            const std::string k_bson_type_string_null("null");
            const std::string k_bson_type_string_document("document");
            const std::string k_bson_type_string_binary_document("binary-document");
            const std::string k_bson_type_string_binary("binary");
            const std::string k_bson_type_string_array("array");
            const std::string k_bson_type_string_unknown("unknown");

            const std::string k_bson_binary_type_string_generic("generic");
            const std::string k_bson_binary_type_string_function("function");
            const std::string k_bson_binary_type_string_binary("binary (old)");
            const std::string k_bson_binary_type_string_uuid("uuid");
            const std::string k_bson_binary_type_string_md5("md5");
            const std::string k_bson_binary_type_string_user_defined("user-defined");
        }; // namespace lj::bson::(anonymous)

        const std::string& type_string(const Type t)
        {

            // All of these strings are declared and defined in the anonymous
            // namespace above.
            switch (t)
            {
                case Type::k_string:
                    return k_bson_type_string_string;
                case Type::k_binary:
                    return k_bson_type_string_binary;
                case Type::k_int32:
                    return k_bson_type_string_int32;
                case Type::k_double:
                    return k_bson_type_string_double;
                case Type::k_int64:
                    return k_bson_type_string_int64;
                case Type::k_timestamp:
                    return k_bson_type_string_timestamp;
                case Type::k_boolean:
                    return k_bson_type_string_boolean;
                case Type::k_null:
                    return k_bson_type_string_null;
                case Type::k_document:
                    return k_bson_type_string_document;
                case Type::k_binary_document:
                    return k_bson_type_string_binary_document;
                case Type::k_array:
                    return k_bson_type_string_array;
                default:
                    break;
            }
            return k_bson_type_string_unknown;
        }

        const std::string& binary_type_string(const Binary_type subtype)
        {
            // All of these strings are declared and defined in the anonymous
            // namespace above.
            switch (subtype)
            {
                case Binary_type::k_bin_generic:
                    return k_bson_binary_type_string_generic;
                case Binary_type::k_bin_function:
                    return k_bson_binary_type_string_function;
                case Binary_type::k_bin_binary:
                    return k_bson_binary_type_string_binary;
                case Binary_type::k_bin_uuid:
                    return k_bson_binary_type_string_uuid;
                case Binary_type::k_bin_md5:
                    return k_bson_binary_type_string_md5;
                case Binary_type::k_bin_user_defined:
                    return k_bson_binary_type_string_user_defined;
                default:
                    break;
            }
            return k_bson_type_string_unknown;
        }

        //=====================================================================
        // Bson path exception.
        //=====================================================================

        std::string Bson_path_exception::str() const
        {
            std::ostringstream oss;
            oss << lj::Exception::str();
            oss << " [for path \"" << path() << "\"]";
            return oss.str();
        }

        //=====================================================================
        // Bson type exception.
        //=====================================================================

        std::string Bson_type_exception::str() const
        {
            std::ostringstream oss;
            oss << lj::Exception::str();
            oss << " [for type \"" << type_string(type()) << "\"";
            if (Type::k_binary == type())
            {
                oss << ", subtype \"" << binary_type_string(binary_type()) << "\"";
            }
            oss << "]";
            return oss.str();
        }


        //=====================================================================
        // Node
        //=====================================================================

        Node::Node() : type_(Type::k_document)
        {
            value_.map_ = new std::map<std::string, Node*>();
        }

        Node::Node(const Type t, const uint8_t* v) : type_(Type::k_null)
        {
            value_.data_ = nullptr;
            set_value(t, v);
        }

        Node::Node(const Node &o) : type_(Type::k_null)
        {
            value_.data_ = nullptr;
            copy_from(o);
        }

        Node::Node(Node&& o) : type_(o.type_),
                value_(o.value_)
        {
            o.value_.data_ = nullptr;
            o.type_ = Type::k_null;
        }

        Node::~Node()
        {
            destroy(true);
        }

        void Node::set_value(const Type t, const uint8_t* v)
        {
            // We have to clear out all of the current value before we can set
            // new values.  we take extra caution to avoid
            // issues with passing values into the same object:
            // e.g. a.set_value(a.type(), a.to_value()).
            uint8_t* old_data = nullptr;
            size_t old_size = 0;
            if (!type_is_nested(type()))
            {
                // We capture this pointer because destroy will lose it
                // and we need to free it later.
                old_size = size();
                old_data = value_.data_;
                destroy(false);
            }
            else
            {
                destroy(true);
            }

            // process the void pointer provided based on the provided type.
            type_ = t;
            if (v || type_ == Type::k_null)
            {
                long long sz = 0;
                switch (type_)
                {
                    case Type::k_string:
                        memcpy(&sz, v, 4);
                        value_.data_ = new uint8_t[sz + 4];
                        memcpy(value_.data_, v, sz + 4);
                        break;
                    case Type::k_binary:
                        memcpy(&sz, v, 4);
                        value_.data_ = new uint8_t[sz + 5];
                        memcpy(value_.data_, v, sz + 5);
                        break;
                    case Type::k_int32:
                        value_.data_ = new uint8_t[4];
                        memcpy(value_.data_, v, 4);
                        break;
                    case Type::k_double:
                    case Type::k_int64:
                    case Type::k_timestamp:
                        value_.data_ = new uint8_t[8];
                        memcpy(value_.data_, v, 8);
                        break;
                    case Type::k_boolean:
                        value_.data_ = new uint8_t[1];
                        memcpy(value_.data_, v, 1);
                        break;
                    case Type::k_null:
                        value_.data_ = NULL;
                        break;
                    case Type::k_document:
                        value_.map_ = new std::map<std::string, Node*>();
                        subdocument(type_, *this, v);
                        break;
                    case Type::k_array:
                        value_.vector_ = new std::vector<Node*>();
                        subdocument(type_, *this, v);
                        break;
                    case Type::k_binary_document:
                        memcpy(&sz, v, 4);
                        value_.data_ = new uint8_t[sz];
                        memcpy(value_.data_, v, sz);
                        break;
                    default:
                        break;
                }
            }
            else if (type_is_nested(type_))
            {
                // we were not passed a value, so create some empty data structures.
                if (Type::k_document == type_)
                {
                    value_.map_ = new std::map<std::string, Node*>();
                }
                else if (Type::k_array == type_)
                {
                    value_.vector_ = new std::vector<Node*>();
                }
            }
            else
            {
                throw Bson_type_exception("NULL pointer passed to non-structural node type.", t);
            }

            // Clean up any old memory.
            if (old_data)
            {
                memset(old_data, 0, old_size);
                delete[] old_data;
            }
        }

        void Node::nullify()
        {
            destroy(true);
        }

        Node& Node::copy_from(const Node& o)
        {
            if (Type::k_document == o.type())
            {
                std::map<std::string, Node*>* tmp = new std::map<std::string, Node*>();
                for (auto iter = o.to_map().begin(); o.to_map().end() != iter; ++iter)
                {
                    Node *ptr = new Node(*(iter->second));
                    tmp->insert(std::pair<std::string, Node*>(iter->first, ptr));
                }
                destroy(true);
                type_ = o.type();
                value_.map_ = tmp;
            }
            else if (Type::k_array == o.type())
            {
                std::vector<Node*>* tmp = new std::vector<Node*>();
                for (auto iter = o.to_vector().begin(); o.to_vector().end() != iter; ++iter)
                {
                    Node *ptr = new Node(*(*iter));
                    tmp->push_back(ptr);
                }
                destroy(true);
                type_ = o.type();
                value_.vector_ = tmp;
            }
            else
            {
                set_value(o.type(), o.to_value());
            }
            return *this;
        }

        Node& Node::operator=(Node&& o)
        {
            if (&o != this)
            {
                destroy(true);
                type_ = o.type_;
                value_ = o.value_;
                o.type_ = Type::k_null;
                o.value_.data_ = nullptr;
            }
            return *this;
        }

        Node* Node::find_or_create_child_documents(const std::list<std::string>& input_list)
        {
            std::list<std::string> parts(input_list);

            // set the root, and loop until all path parts are complete.
            // verifying that each node is a document is handled by the
            // to_map() method.
            Node *n = this;
            while (0 < parts.size())
            {
                if (Type::k_array == n->type())
                {
                    // Check that the position is valid.
                    // Doesn't make sense to pad the vector with empty document
                    // objects, so we don't do that here.
                    int pos = atoi(parts.front().c_str());
                    if(pos < 0 || static_cast<size_t>(pos) >= n->to_vector().size())
                    {
                        throw Bson_path_exception(std::string("Invalid array index ") + parts.front(), parts.front());
                    }

                    // one level deeper.
                    n = n->to_vector().at(pos);
                }
                else
                {
                    // Search for the child by name.
                    auto iter = n->to_map().find(parts.front());
                    if (n->to_map().end() == iter)
                    {
                        // Child not found, so create it.
                        Node* tmp = new Node();
                        n->set_child(parts.front(), tmp);
                        n = tmp;
                    }
                    else
                    {
                        // one level deeper.
                        n = iter->second;
                    }
                }
                parts.pop_front();
            }
            return n;
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
                        if (0 < current.size())
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
                if (0 < current.size())
                {
                    parts.push_back(current);
                }
            }
        };

        Node* Node::path(const std::string& p)
        {
            std::list<std::string> parts;
            split_path(p, parts);
            return find_or_create_child_documents(parts);
        }

        const Node* Node::path(const std::string& p) const
        {
            // split path up for looping.
            std::list<std::string> parts;
            split_path(p, parts);

            // set the root, and loop until all path parts are complete.
            // verifying that each node is a document is handled by the
            // to_map() method.
            const Node *n = this;
            while (0 < parts.size())
            {
                if (Type::k_array == n->type())
                {
                    // Check that the position is valid.
                    int pos = atoi(parts.front().c_str());
                    if (pos < 0 || static_cast<size_t>(pos) >= n->to_vector().size())
                    {
                        return nullptr;
                    }

                    // one level deeper.
                    n = n->to_vector().at(pos);
                }
                else
                {
                    // Search for the child by name.
                    auto iter = n->to_map().find(parts.front());
                    if (n->to_map().end() == iter)
                    {
                        // Child not found, and everything is const, so return null.
                        return nullptr;
                    }

                    // One level deeper.
                    n = iter->second;
                }
                parts.pop_front();
            }
            return n;
        }

        void Node::set_child(const std::string& p, Node* c)
        {
            // split path up for navigation.
            std::list<std::string> parts;
            split_path(p, parts);

            // Check that we got a valid child name.
            if (parts.size() < 1)
            {
                throw Bson_path_exception("Cannot set a child without a child name.", p);
            }

            // This gets the new child's name from the parts.s
            std::string child_name = parts.back();
            parts.pop_back();

            // navigate the structure.
            Node *n = find_or_create_child_documents(parts);

            // Cannot use to_map below because I need a non-const iterator.
            // checking that the found node is a document.
            if (Type::k_document != n->type())
            {
                throw Bson_type_exception("Cannot add a child to a non-document type.", n->type());
            }

            // remove any existing value to keep memory sane.
            auto iter = n->value_.map_->find(child_name);
            if (n->value_.map_->end() != iter)
            {
                if (iter->second == c)
                {
                    //already here, do nothing.
                    return;
                }
                delete iter->second;
                n->value_.map_->erase(iter);
            }

            // At this point we have the target node, we have made sure no
            // existing child exists with the same name, and we need to add
            // the new child onto the node.
            if (c)
            {
                n->value_.map_->insert(std::pair<std::string, Node*>(child_name, c));
            }
        }

        void Node::push_child(const std::string& p, Node* c)
        {
            // Null check.
            if (!c)
            {
                throw Bson_type_exception("Cannot push null as a child.", type());
            }

            // split path up for navigation.
            std::list<std::string> parts;
            split_path(p, parts);

            // Navigate to the target node.
            Node* n = find_or_create_child_documents(parts);

            // Make sure the target node is the correct type.
            if (Type::k_array != n->type())
            {
                throw Bson_type_exception("Cannot push a child to a non-array type.", n->type());
            }

            // push the value to the end.
            n->value_.vector_->push_back(c);
        }

        size_t Node::size() const
        {
            long sz = 0;
            int indx = 0;
            switch (type())
            {
                case Type::k_string:
                    memcpy(&sz, value_.data_, 4);
                    sz += 4;
                    break;
                case Type::k_binary:
                    memcpy(&sz, value_.data_, 4);
                    sz += 5;
                    break;
                case Type::k_int32:
                    sz = 4;
                    break;
                case Type::k_double:
                case Type::k_int64:
                case Type::k_timestamp:
                    sz = 8;
                    break;
                case Type::k_boolean:
                    sz = 1;
                    break;
                case Type::k_null:
                    sz = 0;
                    break;
                case Type::k_array:
                    sz += 5;
                    for (auto iter = to_vector().begin(); to_vector().end() != iter; ++iter)
                    {
                        // Yes, this is ugly, but it avoids string building for
                        // smaller arrays.
                        int key_size = 1;
                        if (indx < 10 && indx > -1)
                        {
                            key_size = 1;
                        }
                        else if (indx < 100 && indx > 9)
                        {
                            key_size = 2;
                        }
                        else if (indx < 1000 && indx > 99)
                        {
                            key_size = 3;
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << indx++;
                            key_size = oss.str().size();
                        }
                        sz += key_size + (*iter)->size() + 2;
                    }
                    break;
                case Type::k_document:
                    sz += 5;
                    for (auto iter = to_map().begin(); to_map().end() != iter; ++iter)
                    {
                        sz += iter->second->size() + iter->first.size() + 2;
                    }
                    break;
                case Type::k_binary_document:
                    memcpy(&sz, value_.data_, 4);
                    break;
                default:
                    break;
            }
            return sz;
        }

        // private, used by to_binary() to copy bytes into a preallocated
        // array.
        size_t Node::copy_to_bson(uint8_t* ptr) const
        {
            size_t sz = size();
            if (Type::k_document == type() || Type::k_binary_document == type())
            {
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for (auto iter = to_map().begin(); to_map().end() != iter; ++iter)
                {
                    Type t = iter->second->type();
                    if (Type::k_binary_document == t)
                    {
                        t = Type::k_document;
                    }
                    memcpy(ptr++, &t, 1);
                    memcpy(ptr, iter->first.c_str(), iter->first.size() + 1);
                    ptr += iter->first.size() + 1;
                    ptr += iter->second->copy_to_bson(ptr);
                }
                *ptr = 0;
            }
            else if (Type::k_array == type())
            {
                int indx = 0;
                memcpy(ptr, &sz, 4);
                ptr += 4;
                for (auto iter = to_vector().begin(); to_vector().end() != iter; ++iter)
                {
                    Type t = (*iter)->type();
                    if (Type::k_binary_document == t)
                    {
                        t = Type::k_document;
                    }
                    memcpy(ptr++, &t, 1);

                    std::ostringstream oss;
                    oss << indx++;
                    std::string key_name = oss.str();
                    memcpy(ptr, key_name.c_str(), key_name.size() + 1);
                    ptr += key_name.size() + 1;
                    ptr += (*iter)->copy_to_bson(ptr);
                }
                *ptr = 0;
            }
            else
            {
                memcpy(ptr, value_.data_, sz);
            }
            return sz;
        }

        // private, used to free the current resources in a standard way.
        void Node::destroy(bool should_delete_value)
        {
            if (type_is_value(type()) && should_delete_value)
            {
                if (value_.data_)
                {
                    // The data section is overwritten for security reasons.
                    size_t sz = size();
                    lj::Wiper<uint8_t[]>::wipe(value_.data_, sz);
                    delete[] value_.data_;
                }
            }
            else if (Type::k_document == type())
            {
                for (auto iter = value_.map_->begin(); value_.map_->end() != iter; ++iter)
                {
                    delete iter->second;
                }
                delete value_.map_;
            }
            else if (Type::k_array == type())
            {
                for (auto iter = value_.vector_->begin(); value_.vector_->end() != iter; ++iter)
                {
                    delete *iter;
                }
                delete value_.vector_;
            }
            type_ = Type::k_null;
            value_.data_ = NULL;
        }


        std::string escape_path(const std::string& input)
        {
            std::string name;
            for (auto iter = input.begin(); input.end() != iter; ++iter)
            {
                if ('/' == *iter)
                {
                    name.push_back('\\');
                }
                else if ('\\' == *iter)
                {
                    name.push_back('\\');
                }
                name.push_back(*iter);
            }
            return name;
        }

        Node* new_string(const std::string& str)
        {
            long sz = str.size() + 1;
            uint8_t* ptr = new uint8_t[sz + 4];;
            memcpy(ptr, &sz, 4);
            memcpy(ptr + 4, str.c_str(), sz);
            Node* new_bson = new Node(Type::k_string, ptr);
            delete[] ptr;
            return new_bson;
        }

        Node* new_boolean(const bool val)
        {
            return new Node(Type::k_boolean, reinterpret_cast<const uint8_t*> (&val));
        }

        Node* new_int32(const int32_t val)
        {
            return new Node(Type::k_int32, reinterpret_cast<const uint8_t*> (&val));
        }

        Node* new_int64(const int64_t val)
        {
            return new Node(Type::k_int64, reinterpret_cast<const uint8_t*> (&val));
        }

        Node* new_uint64(const uint64_t val)
        {
            return new Node(Type::k_int64, reinterpret_cast<const uint8_t*> (&val));
        }

        Node* new_null()
        {
            return new Node(Type::k_null, NULL);
        }

        Node* new_binary(const uint8_t* const val, uint32_t sz, Binary_type subtype)
        {
            uint8_t* ptr = new uint8_t[sz + 5];
            memcpy(ptr, &sz, 4);
            memcpy(ptr + 4, &subtype, 1);
            memcpy(ptr + 5, val, sz);
            Node* new_bson = new Node(Type::k_binary, ptr);
            delete[] ptr;
            return new_bson;
        }

        Node* new_uuid(const Uuid& uuid)
        {
            size_t sz;
            const uint8_t* const d = uuid.data(&sz);
            return new_binary(d, sz, Binary_type::k_bin_uuid);
        }

        Node* new_array()
        {
            return new Node(Type::k_array, NULL);
        }

        std::string as_debug_string(const Node& b, int lvl)
        {
            Binary_type binary_type = Binary_type::k_bin_generic;
            long long l = 0;
            double d = 0.0;
            std::ostringstream buf;

            if (type_is_nested(b.type()))
            {
                // This node can have children, indentation level matters.
                std::string indent;
                for (int h = 0; h < lvl; ++h)
                {
                    indent.append("  ");
                }

                // Caching node size because it can be complicated to compute.
                size_t node_size = b.size();
                if (node_size == 5)
                {
                    return "{(size-4)0(null-1)0}";
                }
                buf << "{(size-4)" << node_size << "\n";

                // The output function is essentially the same for documents
                // and arrays. We create a nested anonymous function for use
                // in the loops below. Except for the capture of the
                // buf object, this is identical to a function in an anonymous
                // namespace.
                auto output_function = [&buf, &indent, &lvl](const std::string& key, const Node* n) {
                    buf << indent;
                    buf << "(type-1)" << type_string(n->type()) << "";
                    buf << "\"(key-" << key.size() + 1 << ")" << escape(key) << "\":";
                    if (type_is_quotable(n->type()))
                    {
                        buf << "\"";
                    }
                    buf << as_debug_string(*n, lvl + 1);
                    if (type_is_quotable(n->type()))
                    {
                        buf << "\"";
                    }
                    buf << ",\n";
                };

                // Handle Documents and Arrays differently.
                if (Type::k_document == b.type())
                {
                    for (auto iter = b.to_map().begin(); b.to_map().end() != iter; ++iter)
                    {
                        output_function(iter->first, iter->second);
                    }
                }
                else
                {
                    int indx = 0;
                    for (auto iter = b.to_vector().begin(); b.to_vector().end() != iter; ++iter)
                    {
                        std::ostringstream oss;
                        oss << indx++;
                        output_function(oss.str(), *iter);
                    }
                }

                //Close the document properly.
                std::string ret_val = buf.str().erase(buf.str().size() - 2).append("\n");
                if (indent.size() >= 2)
                {
                    ret_val.append(indent.erase(indent.size() - 2));
                }
                return ret_val.append("(null-1)0}");            }
            else
            {
                const uint8_t* v = b.to_value();
                switch (b.type())
                {
                    case Type::k_string:
                        memcpy(&l, v, 4);
                        buf << "(size-4)" << l << "(value-" << l << ")" << reinterpret_cast<const char*>(v + 4);
                        return buf.str();
                    case Type::k_binary:
                        memcpy(&l, v, 4);
                        memcpy(&binary_type, v + 4, 1);
                        buf << "(size-4)" << l;
                        buf << "(bin-type-1)" << binary_type_string(binary_type);
                        buf << "(value-" << l << ")";
                        if (Binary_type::k_bin_uuid == binary_type && l == 16)
                        {
                            buf << Uuid(v + 5).str();
                        }
                        else
                        {
                            buf << lj::base64_encode(v + 5, l);
                        }
                        return buf.str();
                    case Type::k_int32:
                        memcpy(&l, v, 4);
                        buf << "(value-4)" << l;
                        return buf.str();
                    case Type::k_double:
                        memcpy(&d, v, 8);
                        buf << "(value-8)" << d;
                        return buf.str();
                    case Type::k_int64:
                    case Type::k_timestamp:
                        memcpy(&l, v, 8);
                        buf << "(value-8)" << l;
                        return buf.str();
                    case Type::k_boolean:
                        memcpy(&l, v, 1);
                        buf << "(value-1)" << ((bool)l);
                        return buf.str();
                    case Type::k_null:
                        buf << "(value-0)";
                        return buf.str();
                    case Type::k_binary_document:
                        return as_debug_string(Node(Type::k_document, v));
                    default:
                        break;
                }
            }
            return std::string();
        }

        std::string as_string(const Node& b)
        {
            Binary_type binary_type = Binary_type::k_bin_generic;
            long long l = 0;
            double d = 0.0;
            std::ostringstream buf;

            if (type_is_nested(b.type()))
            {
                // Caching node size because it can be complicated to compute.
                size_t node_size = b.size();

                // Quickly abort for empty documents and arrays.
                if (node_size == 5)
                {
                    return (Type::k_array == b.type() ? "[]" : "{}");
                }

                buf << (Type::k_array == b.type() ? "[" : "{");

                // The output function is essentially the same for documents
                // and arrays. We create a nested anonymous function for use
                // in the loops below. Except for the capture of the
                // buf object, this is identical to a function in an anonymous
                // namespace.
                auto output_function = [&buf](const std::string& key, const Node* n) {
                    buf << "\"" << escape(key) << "\":";
                    if (type_is_quotable(n->type()))
                    {
                        buf << "\"";
                    }
                    buf << as_string(*n);
                    if (type_is_quotable(n->type()))
                    {
                        buf << "\"";
                    }
                    buf << ", ";
                };

                // Handle Documents and Arrays differently.
                if (Type::k_document == b.type())
                {
                    for (auto iter = b.to_map().begin(); b.to_map().end() != iter; ++iter)
                    {
                        output_function(iter->first, iter->second);
                    }
                }
                else
                {
                    int indx = 0;
                    for (auto iter = b.to_vector().begin(); b.to_vector().end() != iter; ++iter)
                    {
                        std::ostringstream oss;
                        oss << indx++;
                        output_function(oss.str(), *iter);
                    }
                }

                //Close the document properly.
                return buf.str().erase(buf.str().size() - 2).append((Type::k_array == b.type() ? "]" : "}"));
            }
            else
            {
                const uint8_t* v = b.to_value();
                switch (b.type())
                {
                    case Type::k_null:
                        return "null";
                    case Type::k_string:
                        memcpy(&l, v, 4);
                        return std::string(reinterpret_cast<const char*>(v + 4));
                    case Type::k_binary:
                        memcpy(&l, v, 4);
                        memcpy(&binary_type, v + 4, 1);
                        if (Binary_type::k_bin_uuid == binary_type && l == 16)
                        {
                            return Uuid(v + 5).str();
                        }
                        else
                        {
                            return lj::base64_encode(v + 5, l);
                        }
                    case Type::k_int32:
                        memcpy(&l, v, 4);
                        buf << l;
                        return buf.str();
                    case Type::k_double:
                        memcpy(&d, v, 8);
                        buf << d;
                        return buf.str();
                    case Type::k_int64:
                    case Type::k_timestamp:
                        memcpy(&l, v, 8);
                        buf << l;
                        return buf.str();
                    case Type::k_boolean:
                        memcpy(&l, v, 1);
                        buf << ((bool)l);
                        return buf.str();
                    case Type::k_binary_document:
                        return as_string(Node(Type::k_document, v));
                    default:
                        break;
                }
            }
            return std::string();
        }

        std::string as_pretty_json(const Node& b, int lvl)
        {
            std::ostringstream buf;

            if (type_is_nested(b.type()))
            {
                // This node can have children, indentation level matters.
                std::string indent;
                for (int h = 0; h < lvl; ++h)
                {
                    indent.append("  ");
                }

                // Caching node size because it can be complicated to compute.
                size_t node_size = b.size();
                if (node_size == 5)
                {
                    return (Type::k_array == b.type() ? "[]" : "{}");;
                }

                buf << (Type::k_array == b.type() ? "[" : "{") << "\n";

                // The output function is essentially the same for documents
                // and arrays. We create a nested anonymous function for use
                // in the loops below. Except for the capture of the
                // buf object, this is identical to a function in an anonymous
                // namespace.
                auto output_function = [&b, &buf, &indent, &lvl](const std::string& key, const Node* n) {
                    buf << indent;
                    if (Type::k_document == b.type())
                    {
                        buf << "\"" << escape(key) << "\":";
                    }
                    if (!type_is_native(n->type()) && !type_is_nested(n->type()))
                    {
                        buf << "\"";
                        buf << escape(as_pretty_json(*n, lvl + 1));
                        buf << "\"";
                    }
                    else
                    {
                        buf << as_pretty_json(*n, lvl + 1);
                    }
                    buf << ",\n";
                };

                // Handle Documents and Arrays differently.
                if (Type::k_document == b.type())
                {
                    for (auto iter = b.to_map().begin(); b.to_map().end() != iter; ++iter)
                    {
                        output_function(iter->first, iter->second);
                    }
                }
                else
                {
                    int indx = 0;
                    for (auto iter = b.to_vector().begin(); b.to_vector().end() != iter; ++iter)
                    {
                        std::ostringstream oss;
                        oss << indx++;
                        output_function(oss.str(), *iter);
                    }
                }

                //Close the document properly.
                std::string ret_val = buf.str().erase(buf.str().size() - 2).append("\n");
                if (indent.size() >= 2)
                {
                    ret_val.append(indent.erase(indent.size() - 2));
                }
                return ret_val.append((Type::k_array == b.type() ? "]" : "}"));
            }
            else
            {
                const uint8_t* v = b.to_value();
                switch (b.type())
                {
                    case Type::k_binary_document:
                        return as_pretty_json(Node(Type::k_document, v), lvl);
                    default:
                        return as_string(b);
                }
            }
            return std::string();
        }

        int32_t as_int32(const Node& b)
        {
            long l = 0;
            double d = 0.0;
            if (type_is_value(b.type()))
            {
                const uint8_t* v = b.to_value();
                switch (b.type())
                {
                    case Type::k_string:
                        return atoi(reinterpret_cast<const char*>(v + 4));
                    case Type::k_int32:
                        memcpy(&l, v, 4);
                        return (int)l;
                    case Type::k_double:
                        memcpy(&d, v, 8);
                        return (int)d;
                    case Type::k_int64:
                    case Type::k_timestamp:
                        memcpy(&l, v, 8);
                        return (int)l;
                    case Type::k_boolean:
                        memcpy(&l, v, 1);
                        return (int)l;
                    default:
                        break;
                }
            }
            return 0;
        }

        int64_t as_int64(const Node& b)
        {
            int64_t l = 0;
            double d = 0.0;
            if (type_is_value(b.type()))
            {
                const uint8_t* v = b.to_value();
                switch (b.type())
                {
                    case Type::k_string:
                        return atol(reinterpret_cast<const char*>(v + 4));
                    case Type::k_int32:
                        memcpy(&l, v, 4);
                        return l;
                    case Type::k_double:
                        memcpy(&d, v, 8);
                        return (long long)d;
                    case Type::k_int64:
                    case Type::k_timestamp:
                        memcpy(&l, v, 8);
                        return l;
                    case Type::k_boolean:
                        memcpy(&l, v, 1);
                        return l;
                    default:
                        break;
                }
            }
            return 0;
        }

        uint64_t as_uint64(const Node& b)
        {
            uint64_t l = 0;
            double d = 0.0;
            if (type_is_value(b.type()))
            {
                const uint8_t* v = b.to_value();
                switch (b.type())
                {
                    case Type::k_string:
                        return static_cast<uint64_t>(atol(reinterpret_cast<const char*>(v + 4)));
                    case Type::k_int32:
                        memcpy(&l, v, 4);
                        return l;
                    case Type::k_double:
                        memcpy(&d, v, 8);
                        return (long long)d;
                    case Type::k_int64:
                    case Type::k_timestamp:
                        memcpy(&l, v, 8);
                        return l;
                    case Type::k_boolean:
                        memcpy(&l, v, 1);
                        return l;
                    default:
                        break;
                }
            }
            return 0;
        }

        bool as_boolean(const Node& b)
        {
            long l = 0;
            double d = 0.0;
            if (type_is_value(b.type()))
            {
                const uint8_t* v = b.to_value();
                const char* s;
                switch (b.type())
                {
                    case Type::k_string:
                        s = reinterpret_cast<const char*>(v + 4);
                        if (!v)
                        {
                            return false;
                        }
                        if (!s[0])
                        {
                            return false;
                        }
                        if (s[0] == '0' && !s[1])
                        {
                            return false;
                        }
                        if (s[0] == '1' && !s[1])
                        {
                            return true;
                        }
                        if (strlen(s) == 4 &&
                            toupper(s[0]) == 'T' &&
                            toupper(s[1]) == 'R' &&
                            toupper(s[2]) == 'U' &&
                            toupper(s[3]) == 'E')
                        {
                            return true;
                        }
                        break;
                    case Type::k_int32:
                        memcpy(&l, v, 4);
                        return l;
                    case Type::k_double:
                        memcpy(&d, v, 8);
                        return (long)d;
                    case Type::k_int64:
                    case Type::k_timestamp:
                        memcpy(&l, v, 8);
                        return l;
                    case Type::k_boolean:
                        memcpy(&l, v, 1);
                        return l;
                    default:
                        break;
                }
            }
            return false;
        }

        double as_double(const Node& b)
        {
            long l = 0;
            double d = 0.0;
            if (type_is_value(b.type()))
            {
                const uint8_t* v = b.to_value();
                switch (b.type())
                {
                    case Type::k_string:
                        return atof(reinterpret_cast<const char*>(v + 4));
                    case Type::k_int32:
                        memcpy(&l, v, 4);
                        return (double)l;
                    case Type::k_double:
                        memcpy(&d, v, 8);
                        return d;
                    case Type::k_int64:
                    case Type::k_timestamp:
                        memcpy(&l, v, 8);
                        return (double)l;
                    case Type::k_boolean:
                        memcpy(&l, v, 1);
                        return (double)l;
                    default:
                        break;
                }
            }
            return 0.0;
        }

        const uint8_t* as_binary(const Node& b, Binary_type* t, uint32_t* sz)
        {
            if (Type::k_binary != b.type())
            {
                throw Bson_type_exception("Attempt to get non-binary node as binary.", b.type());
            }
            const uint8_t* v = b.to_value();
            memcpy(sz, v, 4);
            memcpy(t, v + 4, 1);
            return v + 5;
        }

        Uuid as_uuid(const Node& b)
        {
            Binary_type t = Binary_type::k_bin_generic;
            uint32_t sz;
            if (Type::k_null != b.type())
            {
                const uint8_t* ptr = reinterpret_cast<const uint8_t*>(as_binary(b, &t, &sz));
                if (Binary_type::k_bin_uuid == t && 16 == sz)
                {
                    return Uuid(ptr);
                }
            }
            return Uuid::k_nil;
        }

        void increment(Node& b, int amount)
        {
            const int64_t v = as_int64(b) + amount;
            b.set_value(Type::k_int64,
                        reinterpret_cast<const uint8_t*> (&v));
        }

        void combine(Node& target, const Node& changes)
        {
            if (Type::k_document == changes.type())
            {
                for (auto iter = changes.to_map().begin();
                        changes.to_map().end() != iter;
                        ++iter)
                {
                    combine(target.nav(iter->first), *(iter->second));
                }
            }
            else
            {
                target = changes;
            }
        }
    }; // namespace lj::bson
}; // namespace lj

std::istream& operator>>(std::istream& is, lj::bson::Node& val)
{
    int read_bytes = 0;
    char len[4];
    while (read_bytes < 4 && is.good())
    {
        is.read(len + read_bytes, 4 - read_bytes);
        read_bytes += is.gcount();
    }

    if (!is.good())
    {
        throw LJ__Exception("Unable to read the length from the input stream.");
    }

    int32_t document_length = *reinterpret_cast<uint32_t*>(len);
    char* document_buffer = new char[document_length];
    memcpy(document_buffer, len, 4);
    while (read_bytes < document_length && is.good())
    {
        is.read(document_buffer + read_bytes, document_length - read_bytes);
        read_bytes += is.gcount();
    }

    if (!is.good())
    {
        throw LJ__Exception("Unable to read document from the input stream.");
    }

    val.set_value(lj::bson::Type::k_document,
            reinterpret_cast<uint8_t*>(document_buffer));

    delete[] document_buffer;

    return is;
}

std::ostream& operator<<(std::ostream& os, lj::bson::Node& val)
{
    size_t sz;
    char* data = reinterpret_cast<char*>(val.to_binary(&sz));

    os.write(data, sz);

    delete[] data;

    return os;
}
