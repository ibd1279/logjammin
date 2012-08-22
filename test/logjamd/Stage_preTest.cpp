/* 
 * File:   Stage_preTest.cpp
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/Stage_json_adapt.h"
#include "logjamd/Stage_pre.h"
#include "logjamd/User.h"
#include "logjamd/constants.h"
#include "logjamd/mock_server.h"
#include <memory>
#include <sstream>

#include "test/logjamd/Stage_preTest_driver.h"

void testBSON()
{
    std::unique_ptr<lj::bson::Node> mode(lj::bson::new_string("BSON\n"));
    // Create the mock request.
    Mock_environment env;
    env.request() << "BSON\n";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();
    
    // Test the next stage.
    TEST_ASSERT(next_stage != NULL);
    TEST_ASSERT(next_stage->name().compare("Authentication") == 0);
    delete next_stage;

    // Test the result
    TEST_ASSERT(env.connection()->user() == NULL);
}

void testJSON()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "json\n";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();

    // Test the next stage.
    TEST_ASSERT(next_stage != NULL);
    TEST_ASSERT(next_stage != &stage);
    TEST_ASSERT(next_stage->name().compare("JSON-Adapter-Execution") == 0);
    logjamd::Stage_json_adapt* adapter =
            dynamic_cast<logjamd::Stage_json_adapt*>(next_stage);
    TEST_ASSERT(adapter != NULL);

    // Clean up the adapter stage.
    delete next_stage;
}

void testHTTP()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "GET /print('Hello') HTTP/1.0";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    stage.logic();

    // Test the result
    // XXX HTTP has a "single-request-per-connection" life cycle,
    // XXX The tests will need to take that into account.
}

void testUnknown()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "rtmp ";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();

    // Test the result.
    std::string expected("{\"message\":\"Unknown mode: rtmp\", \"stage\":\"Pre-connection\", \"success\":0}");
    std::ostringstream oss;
    oss << env.response().rdbuf();
    TEST_ASSERT(oss.str().compare(expected) == 0);
    TEST_ASSERT(next_stage == NULL);
}

int main(int argc, char** argv)
{
    Mock_server_init ctx;
    return Test_util::runner("logjamd::Stage_pre", tests);
}

