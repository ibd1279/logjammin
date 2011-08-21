/* 
 * File:   lj_base64.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Stage_execute.h"
#include "logjamd/constants.h"

void testBasicCommands()
{
    // Test command
    lj::bson::Node request;
    request.set_child("command", lj::bson::new_string("print ('Hello LJ')"));
    // Create the mock request.
    Mock_environment env;
    env.request() << request;

    // perform the stage.
    logjamd::Stage_execute stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();

    // Test the next stage.
    TEST_ASSERT(next_stage != NULL);
    TEST_ASSERT(next_stage != &stage);

    // Test the output.
    lj::bson::Node response;
    env.response() >> response;
    TEST_ASSERT(lj::bson::as_string(response["output/0"]).compare("Hello LJ") == 0);
    std::cout << lj::bson::as_string(response) << std::endl;

    // Clean up the adapter stage.
    delete next_stage;
    
}

int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testBasicCommands),
        {0, ""}
    };
    return Test_util::runner("logjamd::Stage_execute", tests);
}

