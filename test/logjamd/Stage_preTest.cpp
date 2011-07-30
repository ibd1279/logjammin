/* 
 * File:   lj_base64.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Stage_pre.h"
#include <memory>
#include <sstream>

void testBSON()
{
    std::unique_ptr<lj::bson::Node> mode(lj::bson::new_string("BSON\n"));
    // Create the mock request.
    Mock_environment<1> env;
    env.node[0].copy_from(*mode);

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    stage.logic();
}

void testJSON()
{
    std::unique_ptr<lj::bson::Node> mode(lj::bson::new_string("json\n"));
    // Create the mock request.
    Mock_environment<1> env;
    env.node[0].copy_from(*mode);

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    stage.logic();
}

void testHTTP()
{
    std::unique_ptr<lj::bson::Node> mode(lj::bson::new_string("http "));
    // Create the mock request.
    Mock_environment<1> env;
    env.node[0].copy_from(*mode);

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    stage.logic();
}

void testUnknown()
{
    std::unique_ptr<lj::bson::Node> mode(lj::bson::new_string("rtmp "));
    // Create the mock request.
    Mock_environment<1> env;
    env.node[0].copy_from(*mode);

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
    const Test_entry tests[] = {
        PREPARE_TEST(testBSON),
        PREPARE_TEST(testJSON),
        PREPARE_TEST(testHTTP),
        PREPARE_TEST(testUnknown),
        {0, ""}
    };
    return Test_util::runner("logjamd::Stage_auth", tests);
}

