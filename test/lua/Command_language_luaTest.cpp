/* 
 * File:   Command_language_luaTest.cpp
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "lua/Command_language_lua.h"
#include "logjamd/mock_server.h"
#include "test/lua/Command_language_luaTest_driver.h"

#include "test/lua_files.h"
#include <ios>
#include <fstream>

std::string read_file(const std::string& filename)
{
    char* buffer;
    std::ifstream is(path_for(filename),
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

uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,
        0,1,2,3,4,5,6,7,8,9,0,1};

void testBasicCommands()
{
    // Test command
    lj::bson::Node request;
    Mock_environment env;
    lua::Command_language_lua lua(env.connection(), &request);
    env.connection()->set_crypto_key("test", key, 32);

    // Need to get the contents of a file here.
    request.set_child("command",
            lj::bson::new_string(read_file("Command_language_luaTest.lua")));

    // perform the stage. This response object should come from a connection
    // or a stage or something. it shouldn't be built here.
    lj::bson::Node response;
    response.set_child("success", lj::bson::new_boolean(true));
    response.set_child("message", lj::bson::new_string("ok"));
    response.set_child("output", lj::bson::new_array());
    lua.perform(response);

    TEST_ASSERT_MSG(lj::bson::as_string(response.nav("message")),
            lj::bson::as_boolean(response.nav("success")));
}

int main(int argc, char** argv)
{
    return Test_util::runner("lua::Command_language_lua", tests);
}

