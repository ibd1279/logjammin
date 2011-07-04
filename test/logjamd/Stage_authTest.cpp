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
#include "logjamd/User.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/constants.h"

namespace
{
    const lj::Uuid k_auth_method_fake(logjamd::k_auth_method, "password_hash", 13);
    const lj::Uuid k_auth_provider_local(logjamd::k_auth_provider, "local", 5);
};

struct setup_cred
{
    setup_cred(bool seed) :
            user(lj::Uuid())
    {
        creds.set_child("realm",
                lj::bson::new_string("localhost/unittest"));
        creds.set_child("login",
                lj::bson::new_string("joe_developer"));
        creds.set_child("pword",
                lj::bson::new_string("1a!A2b@B"));

        logjamd::Auth_registry::provider(k_auth_provider_local)->method(
                k_auth_method_fake)->change_credentials(&user, &user, *pointer(seed));
    }

    lj::bson::Node* pointer(bool seed)
    {
        lj::bson::Node* ptr = new lj::bson::Node(creds);
        if (seed)
        {
            const uint8_t oseed[8] = {1,2,3,4,5,6,7,8};
            ptr->set_child("oseed", lj::bson::new_binary(
                    oseed,
                    8,
                    lj::bson::Binary_type::k_bin_generic));
        }
        return ptr;
    }

    logjamd::User user;

private:
    lj::bson::Node creds;
};

void testFakeLocalNoSeed()
{
    setup_cred cred(false);
    Mock_environment<1> env;
    env.node[0].set_child("method",
            lj::bson::new_uuid(k_auth_method_fake));
    env.node[0].set_child("provider",
            lj::bson::new_uuid(k_auth_provider_local));
    env.node[0].set_child("data", cred.pointer(false));

    logjamd::Stage_auth stage(env.connection());

    stage.logic();
    lj::bson::Node response;
    env.response() >> response;

    std::string expected_stage("Authentication");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(lj::bson::as_boolean(response["success"]));
    TEST_ASSERT(env.connection()->user() != NULL);
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
            lj::bson::new_uuid(k_auth_method_fake));
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
        PREPARE_TEST(testFakeLocalNoSeed),
        PREPARE_TEST(testUnknownMethod),
        PREPARE_TEST(testUnknownProvider),
        {0, ""}
    };
    return Test_util::runner("logjamd::Stage_auth", tests);
}

