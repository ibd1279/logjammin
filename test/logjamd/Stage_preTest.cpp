/* 
 * File:   Stage_preTest.cpp
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/Stage_pre.h"
#include "logjamd/constants.h"
#include "logjamd/mock_server.h"
#include <memory>
#include <sstream>

#include "test/logjamd/Stage_preTest_driver.h"

void testBSON()
{
    // Create the mock request.
    Mock_env env;
    env.swimmer->sink() << "BSON\n";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    
    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr);
    TEST_ASSERT(next_stage->name().compare("Authentication") == 0);
}

void testHTTP()
{
    // Create the mock request.
    Mock_env env;
    env.swimmer->sink() << "GET /print('Hello') HTTP/1.0";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr);
    TEST_ASSERT(next_stage->name().compare("HTTP-Adapter") == 0);
}

void testUnknown()
{
    // Create the mock request.
    Mock_env env;
    env.swimmer->sink() << "rtmp ";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Test the result.
    std::string expected("{\"message\":\"Unknown mode: rtmp\", \"stage\":\"Pre-connection\", \"success\":0}");
    std::ostringstream oss;
    oss << env.swimmer->source().rdbuf();
    TEST_ASSERT(oss.str().compare(expected) == 0);
    TEST_ASSERT(next_stage == NULL);
}

int main(int argc, char** argv)
{
    Mock_server_init ctx;
    return Test_util::runner("logjamd::Stage_pre", tests);
}

