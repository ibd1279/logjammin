/*!
 \file mock_server.h
 \brief Header for a mocked out logjamd server implementation.
 */
#include "lj/Log.h"
#include "lj/Streambuf_pipe.h"
#include "lj/Wiper.h"
#include "logjamd/Auth_local.h"
#include "logjam/Pool.h"
#include "logjamd/constants.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <istream>
#include <list>
#include <sstream>
#include <string>

const lj::Uuid k_user_id_admin{0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x40, 0x00,
                    0xb2, 0xb3, 0x67, 0x3f,
                    0x1c, 0x1a, 0xf5, 0xda};
const std::string k_user_login_admin("admin");
const std::string k_user_password_admin("1!aA2@Bb");

struct Mock_auth_data
{
    Mock_auth_data(const lj::Uuid& id,
            const std::string& login,
            const std::string& password) :
            n(),
            u(id, login)
    {
        n.set_child("login",
                lj::bson::new_string(logjamd::k_user_login_json));
        n.set_child("password",
                lj::bson::new_string(logjamd::k_user_password_json));
    }

    lj::bson::Node n;
    logjam::User u;
}; // struct Mock_auth_data

struct Mock_server_init
{
    Mock_server_init() :
            json(logjamd::k_user_id_json,
                    logjamd::k_user_login_json,
                    logjamd::k_user_password_json),
            http(logjamd::k_user_id_http,
                    logjamd::k_user_login_http,
                    logjamd::k_user_password_http),
            admin(k_user_id_admin,
                    k_user_login_admin,
                    k_user_password_admin),
            user_repo(),
            auth_repo()
    {
        // Setup users.
        user_repo.store(json.u);
        user_repo.store(http.u);
        user_repo.store(admin.u);

        // Setup auth repo.
        logjam::Authentication_provider* provider = 
                new logjam::Authentication_provider_simple<logjamd::Auth_method_password_hash>(
                        logjamd::k_auth_provider_local);
        auth_repo.enable(provider);
        logjam::Authentication_method& mthd =
                provider->method("bcrypt");

        // setup credentials.
        mthd.change_credential(json.u.id(), json.n);
        mthd.change_credential(http.u.id(), http.n);
        mthd.change_credential(admin.u.id(), admin.n);
    }

    Mock_auth_data json;
    Mock_auth_data http;
    Mock_auth_data admin;
    logjam::User_repository user_repo;
    logjam::Authentication_repository auth_repo;
};

class Swimmer_mock : public logjam::pool::Swimmer
{
public:
    Swimmer_mock(logjam::pool::Lifeguard& lg,
            logjam::Context&& ctx) :
            logjam::pool::Swimmer(lg, std::move(ctx)),
            pipe_(),
            stream_(&pipe_)
    {
    }
    Swimmer_mock(const Swimmer_mock& o) = delete;
    Swimmer_mock(Swimmer_mock&& o) = delete;
    Swimmer_mock& operator=(const Swimmer_mock&& rhs) = delete;
    Swimmer_mock& operator=(Swimmer_mock&& rhs) = delete;
    virtual ~Swimmer_mock() = default;

    virtual void run() override { }
    virtual void stop() override { }
    virtual void cleanup() override { }
    virtual std::iostream& io() override { return stream_; }
    virtual std::ostream& sink() { return pipe_.sink(); }
    virtual std::istream& source() { return pipe_.source(); }
private:
    lj::Streambuf_pipe pipe_;
    std::iostream stream_;
}; // class Swimmer_mock

class Lifeguard_mock : public logjam::pool::Lifeguard
{
public:
    Lifeguard_mock(logjam::pool::Area& a) :
            logjam::pool::Lifeguard(a)
    {
    }
    Lifeguard_mock(const Lifeguard_mock& o) = delete;
    Lifeguard_mock(Lifeguard_mock&& o) = delete;
    Lifeguard_mock& operator=(const Lifeguard_mock& rhs) = delete;
    Lifeguard_mock& operator=(Lifeguard_mock&& rhs) = delete;
    virtual ~Lifeguard_mock() = default;

    virtual void remove(logjam::pool::Swimmer* s) override { delete s; }
    virtual void watch(logjam::pool::Swimmer* s) override { }
    virtual void run() override { }
    virtual void stop() { }
    virtual void cleanup() override { }

    virtual std::unique_ptr<Swimmer_mock> generate_swimmer()
    {
        return std::unique_ptr<Swimmer_mock>(
                new Swimmer_mock(*this,
                        area().spawn_context()));
    }
}; // class Lifeguard_mock

class Area_mock : public logjam::pool::Area
{
public:
    explicit Area_mock(logjam::Environs&& env) :
            logjam::pool::Area(std::move(env))
    {
    }
    Area_mock(const Area_mock& o) = delete;
    Area_mock(Area_mock&& o) = default;
    Area_mock& operator=(const Area_mock& rhs) = delete;
    Area_mock& operator=(Area_mock&& rhs) = default;
    virtual ~Area_mock() = default;

    virtual void prepare() override { }
    virtual void open() override { }
    virtual void close() override { }
    virtual void cleanup() override { }

    virtual std::unique_ptr<Lifeguard_mock> generate_lifeguard()
    {
        return std::unique_ptr<Lifeguard_mock>(
                new Lifeguard_mock(*this));
    }
}; // class Area_mock

struct Mock_env
{
    Mock_env() :
            server(),
            area(logjam::Environs(lj::bson::Node(),
                    &(server.user_repo),
                    &(server.auth_repo))),
            lifeguard(area.generate_lifeguard()),
            swimmer(lifeguard->generate_swimmer())
    {
    }

    Mock_server_init server;
    Area_mock area;
    std::unique_ptr<Lifeguard_mock> lifeguard;
    std::unique_ptr<Swimmer_mock> swimmer;

}; // struct Mock_env
