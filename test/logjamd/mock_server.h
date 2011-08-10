#include "lj/Streambuf_pipe.h"
#include "logjamd/Server.h"
#include "logjamd/Connection.h"

#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>

class Server_mock : public logjamd::Server
{
public:
    Server_mock() : logjamd::Server(new lj::bson::Node())
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
            logjamd::Connection(new Server_mock(), new lj::bson::Node(), stream)
    {
    }
    virtual void start()
    {
    }
};

class Mock_environment
{
public:
    Mock_environment() :
            pipe(),
            connection_(NULL)
    {
    }
    ~Mock_environment()
    {
        if (connection_)
        {
            delete connection_;
        }
    }
    Connection_mock* connection()
    {
        if (!connection_)
        {
            connection_ = new Connection_mock(new std::iostream(&pipe));
        }
        return connection_;
    }
    std::ostream& request()
    {
        return pipe.sink();
    }
    std::istream& response()
    {
        return pipe.source();
    }
private:
    lj::Streambuf_pipe pipe;
    Connection_mock* connection_;
};

