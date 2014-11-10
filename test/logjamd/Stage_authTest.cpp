/*!
 \file test/logjamd/Stage_authTest.cpp
 Copyright (c) 2014, Jason Watson
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * Neither the name of the LogJammin nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
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

