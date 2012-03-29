/*!
 \file lj/Bson_parser.cpp
 \brief LJ Bson parser from a json like string format to bson.
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
#include "lj/Exception.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>

namespace
{
    class Parser_exception : public lj::Exception
    {
    public:
        Parser_exception(const std::string& msg,
                unsigned int col,
                unsigned int line) :
                lj::Exception("Json to Bson", msg),
                col_(col),
                line_(line)
        {
        }

        Parser_exception(const Parser_exception& o) :
                lj::Exception(o),
                col_(o.col_),
                line_(o.line_)
        {
        }

        virtual ~Parser_exception() throw()
        {
        }

        virtual std::string str() const
        {
            std::ostringstream oss;
            oss << lj::Exception::str();
            oss << " [line " << line_;
            oss << " column " << col_;
            oss << "]";
            return oss.str();
        }
    private:
        unsigned int col_;
        unsigned int line_;
    };

    class Parser_state
    {
    public:
        Parser_state(const std::string& input) :
                state_(State::k_pre),
                begin_(NULL),
                current_(NULL),
                end_(NULL),
                node_(NULL),
                parents_(),
                col_(1),
                line_(1)
        {
            size_t input_size = input.size();
            begin_ = new char[input_size];
            end_ = begin_ + input_size;
            memcpy(begin_, input.data(), input_size);
            current_ = begin_;
        }

        ~Parser_state()
        {
            delete[] begin_;
        }

        lj::bson::Node* run()
        {
            do
            {
                if (State::k_pre == state())
                {
                    extract_value();
                }
                else if (State::k_post == state())
                {
                    extract_separator();
                }
                else
                {
                    extract_key();
                }
            } while(next());
            assert(NULL != node_);
            return node_;
        }
    private:
        enum class State
        {
            k_pre,
            k_post,
            k_key
        };

        State state_;
        char* begin_;
        const char* current_;
        const char* end_;
        lj::bson::Node* node_;
        std::stack<lj::bson::Node*> parents_;
        unsigned int col_;
        unsigned int line_;

        // Hidden because it is just the terminating function for a
        // variadic template
        inline bool is(const char c)
        {
            return false;
        }

        State state()
        {
            return state_;
        }

        inline void node_reset()
        {
            node_ = NULL;
        }
        inline lj::bson::Node& node()
        {
            if (!node_)
            {
                node_ = new lj::bson::Node();
            }
            return *node_;
        }
        inline lj::bson::Node& parent()
        {
            return *(parents_.top());
        }
        inline void push_array()
        {
            node().set_value(lj::bson::Type::k_array, NULL);
            parents_.push(node_);
            node_reset();
        }
        inline void push_document()
        {
            node().set_value(lj::bson::Type::k_document, NULL);
            parents_.push(node_);
            node_reset();
        }
        inline void pop()
        {
            assert(0 < parents_.size());
            if (lj::bson::Type::k_array == parents_.top()->type() &&
                NULL != node_)
            {
                *(parents_.top()) << node_;
            }
            node_ = parents_.top();
            parents_.pop();
            state_ = State::k_post;
        }

        inline bool is_valid(ptrdiff_t dist = 0)
        {
            return (current_ + dist) < end_;
        }
        inline const char at(ptrdiff_t dist = 0)
        {
            if (!is_valid(dist))
            {
                throw Parser_exception("Unexpected end of input.",
                        col_,
                        line_);
            }
            return *(current_ + dist);
        }
        bool next(ptrdiff_t dist = 1)
        {
            if (is_valid(dist))
            {
                for (int h = 0; h < dist; ++h)
                {
                    if (is(at(h), '\n'))
                    {
                        line_++;
                        col_ = 1;
                    }
                    else
                    {
                        col_++;
                    }
                }
                current_ += dist;
                return true;
            }
            return false;
        }
        template <class C0, class ...Args>
        bool is(const char c, const C0& test, const Args& ...args)
        {
            return (c == test ? true : is(c, args...));
        }

        inline void extract_null()
        {
            if (is(at(0), 'N', 'n') &&
                is(at(1), 'U', 'u') &&
                is(at(2), 'L', 'l') &&
                is(at(3), 'L', 'l'))
            {
                std::unique_ptr<lj::bson::Node> n(lj::bson::new_null());
                node().copy_from(*n);
            }
            else
            {
                throw Parser_exception("Unexpected value.",
                        col_,
                        line_);
            }
            next(3);
            state_ = State::k_post;
        }
        inline void extract_true()
        {
            if (is(at(0), 'T', 't') &&
                is(at(1), 'R', 'r') &&
                is(at(2), 'U', 'u') &&
                is(at(3), 'E', 'e'))
            {
                std::unique_ptr<lj::bson::Node> n(lj::bson::new_boolean(true));
                node().copy_from(*n);
            }
            else
            {
                throw Parser_exception("Unexpected value.",
                        col_,
                        line_);
            }
            next(3);
            state_ = State::k_post;
        }
        inline void extract_false()
        {
            if (is(at(0), 'F', 'f') &&
                is(at(1), 'A', 'a') &&
                is(at(2), 'L', 'l') &&
                is(at(3), 'S', 's') &&
                is(at(4), 'E', 'e'))
            {
                std::unique_ptr<lj::bson::Node> n(lj::bson::new_boolean(false));
                node().copy_from(*n);
            }
            else
            {
                throw Parser_exception("Unexpected value.",
                        col_,
                        line_);
            }
            next(4);
            state_ = State::k_post;
        }
        void extract_string()
        {
            char quote_character = at();
            std::string buffer;
            while(next())
            {
                // Not in the while statement because next modifies
                // what at() reads from.
                if (at() == quote_character)
                {
                    break;
                }

                if ('\\' == at())
                {
                    switch (at(1))
                    {
                        case '\"':
                            buffer.push_back('\"');
                            break;
                        case '\\':
                            buffer.push_back('\\');
                            break;
                        case '/':
                            buffer.push_back('/');
                            break;
                        case 'b':
                            buffer.push_back('\b');
                            break;
                        case 'f':
                            buffer.push_back('\f');
                            break;
                        case 'n':
                            buffer.push_back('\n');
                            break;
                        case 'r':
                            buffer.push_back('\r');
                            break;
                        case 't':
                            buffer.push_back('\t');
                            break;
                        default:
                            buffer.push_back(at(1));
                            break;
                    }
                    next();
                }
                else
                {
                    buffer.push_back(at());
                }
            }

            std::unique_ptr<lj::bson::Node> n(lj::bson::new_string(buffer));
            node().copy_from(*n);
            state_ = State::k_post;
        }

        void extract_number()
        {
            std::string buffer;
            buffer.push_back(at());

            bool decimal = false;
            if (is(at(), '.'))
            {
                decimal = true;
            }

            do
            {
                switch (at(1))
                {
                    case '.':
                        if (decimal)
                        {
                            throw Parser_exception("Expected a digit.",
                                    col_,
                                    line_);
                        }
                        decimal = true;
                        // fall through.
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '0':
                        buffer.push_back(at(1));
                        break;
                    default:
                        if (decimal)
                        {
                            throw Parser_exception("Decimal not yet supported.",
                                    col_,
                                    line_);
                        }
                        else
                        {
                            std::unique_ptr<lj::bson::Node> n(lj::bson::new_int64(atol(buffer.c_str())));
                            node().copy_from(*n);
                        }
                        state_ = State::k_post;
                        return;
                }
            } while(next());
        }

        void extract_value()
        {
            while (State::k_pre == state_)
            {
                switch (at())
                {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                        // throw away white space before a value.
                        if (!next())
                        {
                            return;
                        }
                        break;
                    case '\'':
                    case '\"':
                        // Hit a quote, so treat as a string.
                        extract_string();
                        break;
                    case 'T':
                    case 't':
                        // true keyword?
                        extract_true();
                        break;
                    case 'F':
                    case 'f':
                        // false keyword?
                        extract_false();
                        break;
                    case 'N':
                    case 'n':
                        // null keyword?
                        extract_null();
                        break;
                    case '-':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '0':
                    case '.':
                        //Number.
                        extract_number();
                        break;
                    case '[':
                        // An Array. Nesting
                        push_array();
                        if (!next())
                        {
                            return;
                        }
                        break;
                    case '{':
                        // A Document. Nesting
                        push_document();
                        state_ = State::k_key;
                        return; // return, not break.
                    case ']':
                        // must be empty
                        pop();
                        break;
                    default:
                        throw Parser_exception("Unexpected character.",
                                col_,
                                line_);
                        break;
                }
            } // while (State::k_pre == state_)
        }

        void extract_separator()
        {
            while (State::k_post == state_)
            {
                switch (at())
                {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                        // throw away white space
                        if (!next())
                        {
                            return;
                        }
                        break;
                    case '}':
                    case ']':
                        pop();
                        if (!next())
                        {
                            return;
                        }
                        break;
                    case ',':
                        if (lj::bson::Type::k_document == parent().type())
                        {
                            node_reset();
                            state_ = State::k_key;
                        }
                        else
                        {
                            parent() << node_;
                            node_reset();
                            state_ = State::k_pre;
                        }
                        return; // return, not break;
                    default:
                        throw Parser_exception("Unexpected character.",
                                col_,
                                line_);
                        break;
                }
            } // while (State::k_post == state_)
        }

        void extract_key()
        {
            bool post_key = false;
            while (State::k_key == state_)
            {
                switch (at())
                {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                        // throw away white space
                        if (!next())
                        {
                            return;
                        }
                        break;
                    case '\'':
                    case '\"':
                        if (post_key)
                        {
                            throw Parser_exception("Unexpected character.",
                                    col_,
                                    line_);
                        }
                        extract_string();
                        parent().set_child(lj::bson::as_string(node()), node_);
                        node().nullify();
                        post_key = true;
                        state_ = State::k_key;
                        if (!next())
                        {
                            return;
                        }
                        break;
                    case ':':
                        state_ = State::k_pre;
                        return;
                    case '}':
                        // must be empty
                        pop();
                        break;
                    default:
                        throw Parser_exception("Unexpected character.",
                                col_,
                                line_);
                        break;
                }
            } // while (State::k_key == state_)
        }
    };
};

lj::bson::Node* lj::bson::parse_string(const std::string val)
{
    Parser_state state(val);
    return state.run();
}
