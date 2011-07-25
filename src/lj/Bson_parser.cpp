#include "Bson.h"
#include "Log.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <stack>

namespace
{
    enum class Expected
    {
        k_key,
        k_value
    };

    enum class Interpret
    {
        k_quoted,
        k_unquoted,
        k_post,
    };

    struct Parser_state
    {
    public:
        Parser_state(const char* b, const char* e) :
                expected(Expected::k_value),
                interpret(Interpret::k_unquoted),
                begin(b),
                end(e),
                c(b),
                n(new lj::bson::Node()),
                parents()
        {
        }
        void push()
        {
            lj::Log::debug.log("bson parser: pushing a node.");
            assert(n != NULL);
            parents.push(n);
            n = NULL;
        }
        void pop()
        {
            lj::Log::debug.log("bson parser: popping a node.");
            assert(0 < parents.size());
            n = parents.top();
            parents.pop();
        }
        lj::bson::Node& node()
        {
            return *node_ptr();
        }
        lj::bson::Node* node_ptr()
        {
            if (!n)
            {
                n = new lj::bson::Node();
            }
            return n;
        }
        void node_reset()
        {
            n = new lj::bson::Node();
        }
        void key()
        {
            lj::Log::debug.log("bson parser: changing to key mode.");
            expected = Expected::k_key;
        }
        void value()
        {
            lj::Log::debug.log("bson parser: changing to value mode.");
            expected = Expected::k_value;
        }
        void quoted()
        {
            lj::Log::debug.log("bson parser: interpreting quoted value.");
            interpret = Interpret::k_quoted;
        }
        void unquoted()
        {
            lj::Log::debug.log("bson parser: interpreting unquoted value.");
            interpret = Interpret::k_unquoted;
        }
        void post()
        {
            lj::Log::debug.log("bson parser: interpreting post value.");
            interpret = Interpret::k_post;
        }
        lj::bson::Node& parent()
        {
            return *parents.top();
        }
        char current()
        {
            return *c;
        }
        char current(ptrdiff_t dist)
        {
            return *(c + dist);
        }
        void advance(ptrdiff_t dist)
        {
            c += dist;
        }
        bool eof(ptrdiff_t dist =0)
        {
            return (c + dist) >= end;
        }

        Expected expected;
        Interpret interpret;
        const char* begin;
        const char* end;
        const char* c;
    private:
        lj::bson::Node* n;
        std::stack<lj::bson::Node*> parents;
    };

    void expecting_value(Parser_state& state)
    {
        std::string buffer;
        char quote_character;
        for (; !state.eof(); state.advance(1))
        {
            if (Interpret::k_unquoted == state.interpret)
            {
                switch (state.current())
                {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                        // throw away white space before a value.
                        break;
                    case '\'':
                    case '\"':
                        // Hit a quote, so treat as a string.
                        quote_character = state.current();
                        state.quoted();
                        break;
                    case 'T':
                    case 't':
                        // true keyword?
                        if (state.eof(3))
                        {
                            throw LJ__Exception("Unexpected end of value.");
                        }
                        if ((state.current(1) == 'R' || state.current(1) == 'r') &&
                            (state.current(2) == 'U' || state.current(2) == 'u') &&
                            (state.current(3) == 'E' || state.current(3) == 'e'))
                        {
                            std::unique_ptr<lj::bson::Node> n(lj::bson::new_boolean(true));
                            state.node().copy_from(*n);
                        }
                        else
                        {
                            throw LJ__Exception("Unexpected value.");
                        }
                        state.advance(3);
                        state.post();
                        break;
                    case 'F':
                    case 'f':
                        // false keyword?
                        if (state.eof(4));
                        {
                            throw LJ__Exception("Unexpected end of value.");
                        }
                        if ((state.current(1) == 'A' || state.current(1) == 'a') &&
                            (state.current(2) == 'L' || state.current(2) == 'l') &&
                            (state.current(3) == 'S' || state.current(3) == 's') &&
                            (state.current(4) == 'E' || state.current(4) == 'e'))
                        {
                            std::unique_ptr<lj::bson::Node> n(lj::bson::new_boolean(false));
                            state.node().copy_from(*n);
                        }
                        else
                        {
                            throw LJ__Exception("Unexpected value.");
                        }
                        state.advance(4);
                        state.post();
                        break;
                    case 'N':
                    case 'n':
                        // true keyword?
                        if (state.eof(3))
                        {
                            throw LJ__Exception("Unexpected end of value.");
                        }
                        if ((state.current(1) == 'U' || state.current(1) == 'u') &&
                            (state.current(2) == 'L' || state.current(2) == 'l') &&
                            (state.current(3) == 'L' || state.current(3) == 'l'))
                        {
                            std::unique_ptr<lj::bson::Node> n(lj::bson::new_null());
                            state.node().copy_from(*n);
                        }
                        else
                        {
                            throw LJ__Exception("Unexpected value.");
                        }
                        state.advance(3);
                        state.post();
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
                        state.post();
                        break;
                    case '[':
                        // An Array. Nesting
                        state.node().set_value(lj::bson::Type::k_array, NULL);
                        state.push();
                        break;
                    case '{':
                        // A Document. Nesting
                        state.node().set_value(lj::bson::Type::k_document, NULL);
                        state.push();
                        state.advance(1); // won't hit loop because of return.
                        state.key();
                        return; // return, not break.
                    case '}':
                    case ']':
                        // must be empty
                        state.pop();
                        state.post();
                        break;
                    default:
                        throw LJ__Exception("unexpected symbol.");
                        break;
                }
                continue;
            }
            else if (Interpret::k_quoted == state.interpret)
            {
                for (; !state.eof() &&
                    state.current() != quote_character;
                    state.advance(1))
                {
                    if ('\\' == state.current() && state.eof(1))
                    {
                        state.advance(1);
                        switch (state.current())
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
                                buffer.push_back(state.current());
                                break;
                        }
                        continue;
                    }
                    buffer.push_back(state.current());
                }
                if (state.eof())
                {
                    LJ__Exception("unexpected end of string.");
                }
                std::unique_ptr<lj::bson::Node> n(lj::bson::new_string(buffer));
                buffer.clear();
                state.node().copy_from(*n);
                state.post();
                continue;
            }
            else if (Interpret::k_post == state.interpret)
            {
                switch (state.current())
                {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                        // throw away white space before a value.
                        break;
                    case '}':
                        state.pop();
                        // still post value.
                        break;
                    case ']':
                        state.parent() << state.node_ptr();
                        state.pop();
                        // still post value.
                        break;
                    case ',':
                        if (lj::bson::Type::k_document == state.parent().type())
                        {
                            state.node_reset();
                            state.unquoted();
                            state.advance(1); // return skips loop incr.
                            state.key();
                            return; // return, not break;
                        }
                        else
                        {
                            state.parent() << state.node_ptr();
                            state.node_reset();
                            state.unquoted();
                        }
                        break;
                    default:
                        throw LJ__Exception("unexpected symbol.");
                        break;
                }
                continue;
            }
        }
    }
};

lj::bson::Node* lj::bson::parse_string(const std::string val)
{
    Parser_state state(val.c_str(), val.c_str() + val.size());
    while (!state.eof())
    {
        if (Expected::k_value == state.expected)
        {
            expecting_value(state);
        }
        else
        {
            throw LJ__Exception("Not Implemented yet.");
        }
    }
    return state.node_ptr();
}
