/* 
 * File:   lj_base64.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/Stage_pre.h"
#include "logjamd/User.h"
#include "logjamd/constants.h"
#include "logjamd/mock_server.h"
#include <memory>
#include <sstream>

void testBSON()
{
    std::unique_ptr<lj::bson::Node> mode(lj::bson::new_string("BSON\n"));
    // Create the mock request.
    Mock_environment env;
    env.request() << "BSON\n";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();
    
    // Test the result
    TEST_ASSERT(next_stage != NULL);
    TEST_ASSERT(next_stage->name().compare(logjamd::Stage_auth(env.connection()).name()) == 0);
    TEST_ASSERT(env.connection()->user() == NULL);
}

void testJSON()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "json\n";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    stage.logic();

    // Test the result
    TEST_ASSERT(env.connection()->user() != NULL);
    TEST_ASSERT(env.connection()->user()->id() == logjamd::k_user_id_json);
    TEST_ASSERT(env.connection()->user()->login() == logjamd::k_user_login_json);
}

void testHTTP()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "http ";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    stage.logic();

    // Test the result
    TEST_ASSERT(env.connection()->user() != NULL);
    TEST_ASSERT(env.connection()->user()->id() == logjamd::k_user_id_http);
    TEST_ASSERT(env.connection()->user()->login() == logjamd::k_user_login_http);
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
    const Test_entry tests[] = {
        PREPARE_TEST(testBSON),
        PREPARE_TEST(testJSON),
        PREPARE_TEST(testHTTP),
        PREPARE_TEST(testUnknown),
        {0, ""}
    };
    return Test_util::runner("logjamd::Stage_auth", tests);
}

