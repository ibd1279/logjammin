/* 
 * luaTest.cpp
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "lua/Command_language_lua.h"
#include "logjamd/mock_server.h"
#include "test/lua/luaTest_driver.h"

#include "test/lua_files.h"
#include <ios>
#include <fstream>

template <class T>
class Invoke_script_test
{
public:
    Invoke_script_test() :
            request_(),
            env_(),
            language_(env_.connection(), &request_)
    {
    }

    ~Invoke_script_test()
    {
    }

    inline lj::bson::Node& request()
    {
        return request_;
    }

    inline Mock_environment& env()
    {
        return env_;
    }

    inline T& language()
    {
        return language_;
    }

    lj::bson::Node perform(const std::string& path)
    {
        request_.set_child("command",
                lj::bson::new_string(read_file(path)));

        // This response object should come from a
        // connection or a stage or something. it shouldn't be built here.
        lj::bson::Node response;
        response.set_child("success", lj::bson::new_boolean(true));
        response.set_child("message", lj::bson::new_string("ok"));
        response.set_child("output", lj::bson::new_array());
        language_.perform(response);

        TEST_ASSERT_MSG(lj::bson::as_string(response.nav("message")),
                lj::bson::as_boolean(response.nav("success")));

        return response;
    }
private:
    lj::bson::Node request_;
    Mock_environment env_;
    T language_;

    std::string read_file(const std::string& filename)
    {
        char* buffer;
        std::ifstream is(filename,
                std::ios_base::binary | std::ios_base::in);
        is.seekg(0, std::ios_base::end);
        int length = is.tellg();
        is.seekg(0, std::ios_base::beg);

        buffer = new char[length];
        int loc = 0;
        while(loc < length)
        {
            is.read(buffer, length - loc);
            loc = is.tellg();
        }
        is.close();
        std::string tmp(buffer, length);
        delete buffer;
        return tmp;
    }
};

uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,
        0,1,2,3,4,5,6,7,8,9,0,1};

void testBasicCommands()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    harness.env().connection()->set_crypto_key("test", key, 32);
    lj::bson::Node response(
            harness.perform(path_for("Command_language_luaTest.lua")));
}

void testBson()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    harness.env().connection()->set_crypto_key("test", key, 32);
    lj::bson::Node response(
            harness.perform(path_for("BsonTest.lua")));
}

void testUuid()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    harness.env().connection()->set_crypto_key("test", key, 32);
    lj::bson::Node response(
            harness.perform(path_for("UuidTest.lua")));
}

void testDocument()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    harness.env().connection()->set_crypto_key("test", key, 32);
    lj::bson::Node response(
            harness.perform(path_for("DocumentTest.lua")));
}

int main(int argc, char** argv)
{
    return Test_util::runner("lua::Command_language_lua", tests);
}

