/*!
 \file mock_server.h
 \brief Header for a mocked out logjamd server implementation.
 */
#include "lj/Log.h"
#include "lj/Streambuf_pipe.h"
#include "logjamd/Auth.h"
#include "logjamd/Auth_local.h"
#include "logjamd/Server.h"
#include "logjamd/Connection.h"
#include "logjamd/constants.h"
#include "cryptopp/secblock.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <istream>
#include <map>
#include <sstream>
#include <string>

const lj::Uuid k_auth_method_password_hash(logjamd::k_auth_method,
        "password_hash",
        13);
const lj::Uuid k_auth_provider_local(logjamd::k_auth_provider,
        "local",
        5);
const lj::Uuid k_user_id_admin{0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x40, 0x00,
                    0xb2, 0xb3, 0x67, 0x3f,
                    0x1c, 0x1a, 0xf5, 0xda};
const std::string k_user_login_admin("admin");
const std::string k_user_password_admin("1!aA2@Bb");

struct Json_auth
{
    Json_auth() : n(), u(logjamd::k_user_id_json, logjamd::k_user_login_json)
    {
        n.set_child("login",
                lj::bson::new_string(logjamd::k_user_login_json));
        n.set_child("password",
                lj::bson::new_string(logjamd::k_user_password_json));

        // Setup the user in the auth registry.
        lj::log::disable<lj::Debug>();
        auto provider = logjamd::Auth_registry::provider(k_auth_provider_local);
        assert(provider);
        auto method = provider->method(k_auth_method_password_hash);
        assert(method);
        method->change_credentials(&u, &u, n);
        lj::log::enable<lj::Debug>();
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
        lj::log::disable<lj::Debug>();
        logjamd::Auth_registry::provider(k_auth_provider_local)->
                method(k_auth_method_password_hash)->
                change_credentials(&u, &u, n);
        lj::log::enable<lj::Debug>();
    }

    lj::bson::Node n;
    logjamd::User u;
};

struct Admin_auth
{
    Admin_auth() : n(),
            u(k_user_id_admin, k_user_login_admin)
    {
        n.set_child("login",
                lj::bson::new_string(k_user_login_admin));
        n.set_child("password",
                lj::bson::new_string(k_user_password_admin));

        // Setup the user in the auth registry.
        lj::log::disable<lj::Debug>();
        logjamd::Auth_registry::provider(k_auth_provider_local)->
                method(k_auth_method_password_hash)->
                change_credentials(&u, &u, n);
        lj::log::enable<lj::Debug>();
    }

    lj::bson::Node n;
    logjamd::User u;
};

struct Mock_server_init
{
    logjamd::Auth_provider_local provider_local;
    Json_auth json;
    Http_auth http;
    Admin_auth admin;
};

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
private:
    logjamd::Server* server_;
    std::map<std::string, CryptoPP::SecBlock<uint8_t>*> keys_;
public:
    Connection_mock(std::iostream* stream) :
            logjamd::Connection(new Server_mock(), new lj::bson::Node(), stream),
            server_(&server()),
            keys_()
    {
    }
    ~Connection_mock()
    {
        for (auto iter = keys_.begin();
                keys_.end() != iter;
                ++iter)
        {
            delete (*iter).second;
        }
        delete server_;
    }
    virtual void start()
    {
    }
    void set_crypto_key(const std::string& identifier,
            const void* key,
            int sz)
    {
        keys_[identifier] = new CryptoPP::SecBlock<uint8_t>(
                static_cast<const uint8_t*>(key),
                sz);
    }
    const void* get_crypto_key(
            const std::string& identifier,
            int* sz)
    {
        auto iter = keys_.find(identifier);
        if(keys_.end() != iter)
        {
            *sz = (*iter).second->size();
            return (*iter).second->data();
        }
        else
        {
            *sz = 0;
            return NULL;
        }
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

