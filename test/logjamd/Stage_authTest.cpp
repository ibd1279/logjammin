/* 
 * File:   Stage_authTest.cpp
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Auth_local.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/constants.h"

#include "test/logjamd/Stage_authTest_driver.h"

void testSuccess()
{
    // Set up testing environment.
    Mock_env env;

    // Create the mock request.
    lj::bson::Node n;
    n.set_child("method", lj::bson::new_string(logjamd::k_auth_method_password));
    n.set_child("provider", lj::bson::new_string(logjamd::k_auth_provider_local));
    n.set_child("data", new lj::bson::Node(env.server.admin.n));
    env.swimmer->sink() << n;

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(new logjamd::Stage_auth);
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    lj::bson::Node response;
    env.swimmer->source() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr); // connection has next stage.
    TEST_ASSERT(next_stage->name().compare("Execution") == 0);

    // Test the response.
    std::string expected_stage("Authentication");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(env.swimmer->context().user().id() == env.server.admin.u.id());
}

void testBadData()
{
    // Set up testing environment.
    Mock_env env;

    // Create the mock request.
    env.server.admin.n.set_child("password", lj::bson::new_string("wrong-password."));
    lj::bson::Node n;
    n.set_child("method", lj::bson::new_string(logjamd::k_auth_method_password));
    n.set_child("provider", lj::bson::new_string(logjamd::k_auth_provider_local));
    n.set_child("data", new lj::bson::Node(env.server.admin.n));
    env.swimmer->sink() << n;

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(new logjamd::Stage_auth());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    lj::bson::Node response;
    env.swimmer->source() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr); // connection has next stage.
    TEST_ASSERT(next_stage->name().compare("Authentication") == 0);

    // Test the response.
    std::string expected_stage("Authentication");
    std::string expected_msg("Authentication failed.");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(!lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(expected_msg.compare(lj::bson::as_string(response["message"])) == 0);
    TEST_ASSERT(env.swimmer->context().user().id() == logjam::User::k_unknown.id());
}

void testUnknownMethod()
{
    // Set up testing environment.
    Mock_env env;

    // Create the mock request.
    lj::bson::Node n;
    n.set_child("method", lj::bson::new_string("WUT?"));
    n.set_child("provider", lj::bson::new_string(logjamd::k_auth_provider_local));
    n.set_child("data", new lj::bson::Node(env.server.admin.n));
    env.swimmer->sink() << n;

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(new logjamd::Stage_auth());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    lj::bson::Node response;
    env.swimmer->source() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr); // connection has next stage.
    TEST_ASSERT(next_stage->name().compare("Authentication") == 0);

    // Test the response.
    std::string expected_stage("Authentication");
    std::string expected_msg("Unknown auth method.");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(!lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(expected_msg.compare(lj::bson::as_string(response["message"])) == 0);
    TEST_ASSERT(env.swimmer->context().user().id() == logjam::User::k_unknown.id());
}

void testUnknownProvider()
{
    // Set up testing environment.
    Mock_env env;

    // Create the mock request.
    lj::bson::Node n;
    n.set_child("method", lj::bson::new_string(logjamd::k_auth_method_password));
    n.set_child("provider", lj::bson::new_string("WUT?"));
    n.set_child("data", new lj::bson::Node(env.server.admin.n));
    env.swimmer->sink() << n;

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(new logjamd::Stage_auth());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    lj::bson::Node response;
    env.swimmer->source() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr); // connection has next stage.
    TEST_ASSERT(next_stage->name().compare("Authentication") == 0);

    // Test the response.
    std::string expected_stage("Authentication");
    std::string expected_msg("Unknown auth provider.");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(!lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(expected_msg.compare(lj::bson::as_string(response["message"])) == 0);
    TEST_ASSERT(env.swimmer->context().user().id() == logjam::User::k_unknown.id());
}

int main(int argc, char** argv)
{
    Mock_server_init ctx;
    return Test_util::runner("logjamd::Stage_auth", tests);
}

