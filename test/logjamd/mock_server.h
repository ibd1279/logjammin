#include "lj/Streambuf_pipe.h"
#include "logjamd/Auth.h"
#include "logjamd/Auth_local.h"
#include "logjamd/Server.h"
#include "logjamd/Connection.h"
#include "logjamd/constants.h"

#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>

const lj::Uuid k_auth_method_password_hash(logjamd::k_auth_method,
        "password_hash",
        13);
const lj::Uuid k_auth_provider_local(logjamd::k_auth_provider,
        "local",
        5);

struct Json_auth
{
    Json_auth() : n(), u(logjamd::k_user_id_json, logjamd::k_user_login_json)
    {
        n.set_child("login",
                lj::bson::new_string(logjamd::k_user_login_json));
        n.set_child("password",
                lj::bson::new_string(logjamd::k_user_password_json));

        // Setup the user in the auth registry.
        logjamd::Auth_registry::provider(k_auth_provider_local)->
                method(k_auth_method_password_hash)->
                change_credentials(&u, &u, n);
    }

    lj::bson::Node n;
    logjamd::User u;
};

struct Http_auth
{
    Http_auth() : n(), u(logjamd::k_user_id_http, logjamd::k_user_login_http)
    {
        n.set_child("login",
                lj::bson::new_string(logjamd::k_user_login_http));
        n.set_child("password",
                lj::bson::new_string(logjamd::k_user_password_http));

        // Setup the user in the auth registry.
        logjamd::Auth_registry::provider(k_auth_provider_local)->
                method(k_auth_method_password_hash)->
                change_credentials(&u, &u, n);
    }

    lj::bson::Node n;
    logjamd::User u;
};

namespace auth_provider_namespace
{
    logjamd::Auth_provider_local provider_local;
    Json_auth json;
    Http_auth http;
}

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

