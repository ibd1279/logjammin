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

namespace
{
    const lj::Uuid k_auth_method_password_hash(logjamd::k_auth_method, "password_hash", 13);
    const lj::Uuid k_auth_provider_local(logjamd::k_auth_provider, "local", 5);

    struct creds
    {
        creds(bool alt_password) : n(), u(lj::Uuid(12), "admin")
        {
            n.set_child("login",
                    lj::bson::new_string("admin"));
            if (alt_password)
            {
                n.set_child("password",
                        lj::bson::new_string("abc123"));
            }
            else
            {
                n.set_child("password",
                        lj::bson::new_string("1!aA2@bB"));
            }
        }

        lj::bson::Node n;
        logjamd::User u;
    };

    logjamd::Auth_provider_local _;
};

void testSuccess()
{
    // Create a user for testing.
    creds first(false);
    logjamd::Auth_registry::provider(k_auth_provider_local)->
            method(k_auth_method_password_hash)->
            change_credentials(&first.u, &first.u, first.n);

    // Create the mock request.
    Mock_environment<1> env;
    env.node[0].set_child("method",
            lj::bson::new_uuid(k_auth_method_password_hash));
    env.node[0].set_child("provider",
            lj::bson::new_uuid(k_auth_provider_local));
    env.node[0].set_child("data", new lj::bson::Node(first.n));

    // perform the stage.
    logjamd::Stage_auth stage(env.connection());
    stage.logic();
    lj::bson::Node response;
    env.response() >> response;

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
    creds first(false);
    logjamd::Auth_registry::provider(k_auth_provider_local)->
            method(k_auth_method_password_hash)->
            change_credentials(&first.u, &first.u, first.n);

    // Create the mock request.
    creds second(true);
    Mock_environment<1> env;
    env.node[0].set_child("method",
            lj::bson::new_uuid(k_auth_method_password_hash));
    env.node[0].set_child("provider",
            lj::bson::new_uuid(k_auth_provider_local));
    env.node[0].set_child("data", new lj::bson::Node(second.n));

    // perform the stage.
    logjamd::Stage_auth stage(env.connection());
    stage.logic();
    lj::bson::Node response;
    env.response() >> response;

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
    Mock_environment<1> env;
    env.node[0].set_child("method",
            lj::bson::new_uuid(logjamd::k_logjamd_root));
    env.node[0].set_child("provider",
            lj::bson::new_uuid(k_auth_provider_local));
    logjamd::Stage_auth stage(env.connection());

    stage.logic();
    lj::bson::Node response;
    env.response() >> response;

    std::string expected_stage("Authentication");
    std::string expected_msg("Unknown auth method.");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(!lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(expected_msg.compare(lj::bson::as_string(response["message"])) == 0);
    TEST_ASSERT(env.connection()->user() == NULL);
}

void testUnknownProvider()
{
    Mock_environment<1> env;
    env.node[0].set_child("method",
            lj::bson::new_uuid(k_auth_method_password_hash));
    env.node[0].set_child("provider",
            lj::bson::new_uuid(logjamd::k_logjamd_root));
    logjamd::Stage_auth stage(env.connection());

    stage.logic();
    lj::bson::Node response;
    env.response() >> response;

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

