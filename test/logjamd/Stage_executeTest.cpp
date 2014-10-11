/* 
 * File:   Stage_executeTest.cpp
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Stage_execute.h"
#include "logjamd/constants.h"

#include "test/logjamd/Stage_executeTest_driver.h"

void testBasicCommands()
{
    // Set up testing environment.
    Mock_env env;

    // Test command
    lj::bson::Node request;
    request.set_child("command", lj::bson::new_string( "print ('Hello LJ')\n\
print ('testing', 'foobar', Uuid:new('{444df00e-95ce-4dd6-8f1c-6dc8b96f92d9}'))\n\
print (Document:new())\n\
print (Uuid:new())"));

    // Put the data on the pipe.
    env.swimmer->sink() << request;

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_execute());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    lj::bson::Node response;
    env.swimmer->source() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr);

    // Test the output.
    std::cout << lj::bson::as_string(response) << std::endl;
    TEST_ASSERT(lj::bson::as_string(response["output/0"]).compare("Hello LJ") == 0);
    TEST_ASSERT(lj::bson::as_string(response["output/1"]).compare("testing\tfoobar\t{444df00e-95ce-4dd6-8f1c-6dc8b96f92d9}") == 0);
}

int main(int argc, char** argv)
{
    return Test_util::runner("logjamd::Stage_execute", tests);
}

