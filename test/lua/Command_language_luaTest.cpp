/* 
 * File:   Command_language_luaTest.cpp
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "lua/Command_language_lua.h"
#include "logjamd/mock_server.h"

#include "test/lua/Command_language_luaTest_driver.h"

std::string basic_commands(
#include "test/lua/Command_language_luaTest_lua.h"
);
void testBasicCommands()
{
    // Test command
    lj::bson::Node request;
    Mock_environment env;
    lua::Command_language_lua lua(env.connection(), &request);

    // Need to get the contents of a file here.
    request.set_child("command", lj::bson::new_string(basic_commands));

    // perform the stage.
    lj::bson::Node response;
    response.set_child("output", lj::bson::new_array());
    lua.perform(response);

    std::cout << lj::bson::as_pretty_json(response) << std::endl;
}

int main(int argc, char** argv)
{
    return Test_util::runner("lua::Command_language_lua", tests);
}

