/* 
 * File:   Command_language_luaTest.cpp
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "lua/Command_language_lua.h"
#include "logjamd/mock_server.h"

#include "test/lua/Command_language_luaTest_driver.h"

void testBasicCommands()
{
    // Test command
    lj::bson::Node request;
    Mock_environment env;
    lua::Command_language_lua lua(env.connection(), &request);
    request.set_child("command", lj::bson::new_string("\
print('Hello World')\n\
d = Document:new()\n\
print(d:id())\n\
d:rekey(12345)\n\
print(d:key())\n\
    "));

    // perform the stage.
    lj::bson::Node response;
    response.set_child("output", lj::bson::new_array());
    lua.perform(response);

    TEST_ASSERT(lj::bson::as_string(response["output/0"]).compare("Hello World") == 0);
    TEST_ASSERT(lj::bson::as_string(response["output/1"]).compare("{00000000-0000-0000-0000-000000000000}") == 0);
    TEST_ASSERT(lj::bson::as_string(response["output/2"]).compare("12345") == 0);
    std::cout << lj::bson::as_pretty_json(response) << std::endl;
}

int main(int argc, char** argv)
{
    return Test_util::runner("lua::Command_language_lua", tests);
}

