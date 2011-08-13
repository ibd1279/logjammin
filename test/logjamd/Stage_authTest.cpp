/* 
 * File:   lj_base64.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Auth.h"
#include "logjamd/Auth_local.h"
#include "logjamd/User.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/constants.h"

void testSuccess()
{
    // Create a user for testing.
    Admin_auth first;

    // Create the mock request.
    Mock_environment env;
    lj::bson::Node n;
    n.set_child("method", lj::bson::new_uuid(k_auth_method_password_hash));
    n.set_child("provider", lj::bson::new_uuid(k_auth_provider_local));
    n.set_child("data", new lj::bson::Node(first.n));
    env.request() << n;

    // perform the stage.
    logjamd::Stage_auth stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();
    lj::bson::Node response;
    env.response() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != NULL); // connection has next stage.
    TEST_ASSERT(next_stage != &stage); // next stage is not auth.
    TEST_ASSERT(next_stage->name().compare("Execution") == 0);
    delete next_stage;

    // Test the response.
    std::string expected_stage("Authentication");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(env.connection()->user() != NULL);
    TEST_ASSERT(env.connection()->user()->id() == first.u.id());
}

void testBadData()
{
    // Create a user for testing.
    Admin_auth first;

    // Create the mock request.
    first.n.set_child("password", lj::bson::new_string("wrong-password."));
    Mock_environment env;
    lj::bson::Node n;
    n.set_child("method", lj::bson::new_uuid(k_auth_method_password_hash));
    n.set_child("provider", lj::bson::new_uuid(k_auth_provider_local));
    n.set_child("data", new lj::bson::Node(first.n));
    env.request() << n;

    // perform the stage.
    logjamd::Stage_auth stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();
    lj::bson::Node response;
    env.response() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != NULL); // connection has next stage.
    TEST_ASSERT(next_stage == &stage); // next stage should be same auth.

    // Test the response.
    std::string expected_stage("Authentication");
    std::string expected_msg("Authentication failed.");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(!lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(expected_msg.compare(lj::bson::as_string(response["message"])) == 0);
    TEST_ASSERT(env.connection()->user() == NULL);
}

void testUnknownMethod()
{
    // create the mock request.
    Mock_environment env;
    lj::bson::Node n;
    n.set_child("method",
            lj::bson::new_uuid(logjamd::k_logjamd_root));
    n.set_child("provider",
            lj::bson::new_uuid(k_auth_provider_local));
    env.request() << n;

    // perform the stage.
    logjamd::Stage_auth stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();
    lj::bson::Node response;
    env.response() >> response;

    // Test the response.
    TEST_ASSERT(next_stage != NULL);
    TEST_ASSERT(next_stage == &stage);
    std::string expected_stage("Authentication");
    std::string expected_msg("Unknown auth method.");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(!lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(expected_msg.compare(lj::bson::as_string(response["message"])) == 0);
    TEST_ASSERT(env.connection()->user() == NULL);
}

void testUnknownProvider()
{
    // Create the mock request.
    Mock_environment env;
    lj::bson::Node n;
    n.set_child("method", lj::bson::new_uuid(k_auth_method_password_hash));
    n.set_child("provider", lj::bson::new_uuid(logjamd::k_logjamd_root));
    env.request() << n;

    // Perform the stage.
    logjamd::Stage_auth stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();
    lj::bson::Node response;
    env.response() >> response;

    // Test the response.
    TEST_ASSERT(next_stage != NULL);
    TEST_ASSERT(next_stage == &stage);
    std::string expected_stage("Authentication");
    std::string expected_msg("Unknown auth provider.");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(!lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(expected_msg.compare(lj::bson::as_string(response["message"])) == 0);
    TEST_ASSERT(env.connection()->user() == NULL);
}

int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testSuccess),
        PREPARE_TEST(testBadData),
        PREPARE_TEST(testUnknownMethod),
        PREPARE_TEST(testUnknownProvider),
        {0, ""}
    };
    return Test_util::runner("logjamd::Stage_auth", tests);
}

