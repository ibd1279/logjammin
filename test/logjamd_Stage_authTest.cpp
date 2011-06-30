/* 
 * File:   lj_base64.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "logjamd_mock_server.h"
#include "logjamd/Stage_auth.h"

namespace
{
    const lj::Uuid k_logjamd(lj::Uuid::k_nil, "logjamd", 7);
    const lj::Uuid k_auth_method(k_logjamd, "auth_method", 11);
    const lj::Uuid k_auth_method_fake(k_auth_method, "fake", 4);

    const lj::Uuid k_auth_provider(k_logjamd, "auth_provider", 13);
    const lj::Uuid k_auth_provider_local(k_auth_provider, "local", 5);
};

void testFakeLocal()
{
    Mock_environment<1> env;
    env.node[0].set_child("method", lj::bson::new_uuid(k_auth_method_fake));
    env.node[0].set_child("provider", lj::bson::new_uuid(k_auth_provider_local));
    logjamd::Stage_auth stage(env.connection());

    stage.logic();

    lj::bson::Node response;
    env.response() >> response;

    std::string expected_stage("Authentication");
    TEST_ASSERT(expected_stage.compare(lj::bson::as_string(response["stage"])) == 0);
    TEST_ASSERT(lj::bson::as_boolean(response["success"]));

}

void testUnknownMethod()
{
    Mock_environment<1> env;
    env.node[0].set_child("method", lj::bson::new_uuid(k_auth_method_fake));
    env.node[0].set_child("provider", lj::bson::new_uuid(k_logjamd));
    logjamd::Stage_auth stage(env.connection());

    stage.logic();
}

void testUnknownProvider()
{
}

int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testFakeLocal),
        PREPARE_TEST(testUnknownMethod),
        PREPARE_TEST(testUnknownProvider),
        {0, ""}
    };
    return Test_util::runner("lj::Base64", tests);
}

