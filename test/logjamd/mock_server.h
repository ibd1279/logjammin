#include "logjamd/Server.h"
#include "logjamd/Connection.h"

#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>

class Server_mock : public logjamd::Server
{
public:
    Server_mock() : logjamd::Server(new lj::Document())
    {
    }
    virtual void startup()
    {
    }
    virtual void listen()
    {
    }
    virtual void shutdown()
    {
    }
};

class Connection_mock : public logjamd::Connection
{
public:
    Connection_mock(std::iostream* stream) :
            logjamd::Connection(new Server_mock(), new lj::Document(), stream)
    {
    }
    virtual void start()
    {
    }
};

struct Streambuf_mock : public std::streambuf
{
    Streambuf_mock(char* buffer, size_t sz) :
            ss(std::ios_base::out|std::ios_base::in|std::ios_base::binary),
            buffer_(buffer)
    {
        this->setp(NULL, NULL);
        this->setg(buffer, buffer, buffer + sz);
    }

    ~Streambuf_mock()
    {
        if (buffer_)
        {
            delete[] buffer_;
        }
    }

    virtual int underflow()
    {
        return EOF;
    }

    virtual int overflow(int c = EOF)
    {
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            ss.put(traits_type::to_char_type(c));
        }
        return c;
    }
    std::stringstream ss;
    char* buffer_;
};

template <unsigned LENGTH>
char* nodes_to_cstr(lj::bson::Node (&nodes)[LENGTH], size_t* sz)
{
    *sz = 0;
    for (unsigned h = 0; h < LENGTH; ++h)
    {
        *sz += nodes[h].size();
    }

    char* buffer = new char[*sz];
    size_t offset = 0;
    for (unsigned h = 0; h < LENGTH; ++h)
    {
        if (lj::bson::type_is_nested(nodes[h].type()))
        {
            size_t node_size = nodes[h].size();
            uint8_t* node_bytes = nodes[h].to_binary();
            memcpy(buffer + offset, node_bytes, node_size);
            delete[] node_bytes;
            offset += node_size;
        }
        else
        {
            std::string tmp = lj::bson::as_string(nodes[h]);
            size_t node_size = tmp.size() + 1;
            memcpy(buffer + offset, tmp.c_str(), node_size);
            offset += node_size;
        }
    }
    return buffer;
}

template <unsigned LENGTH>
class Mock_environment
{
public:
    Mock_environment() :
            node(),
            input_buffer(NULL),
            sb(NULL),
            connection_(NULL)
    {
    }
    ~Mock_environment()
    {
        if (connection_)
        {
            delete connection_;
        }

        if (sb)
        {
            delete sb;
        }
    }
    Connection_mock* connection()
    {
        if (!connection_)
        {
            size_t sz;
            input_buffer = nodes_to_cstr(node, &sz);
            sb = new Streambuf_mock(input_buffer, sz);
            connection_ = new Connection_mock(new std::iostream(sb));
        }
        return connection_;
    }
    std::iostream& response()
    {
        return sb->ss;
    }
    lj::bson::Node node[LENGTH];
private:
    char* input_buffer;
    Streambuf_mock* sb;
    Connection_mock* connection_;
};

