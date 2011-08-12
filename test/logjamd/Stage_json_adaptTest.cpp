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
#include "logjamd/Stage_json_adapt.h"
#include "logjamd/Stage_pre.h"
#include "logjamd/constants.h"

void testJsonAuthInsecure()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "json\nTesting\n";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();
    
    // Test the result
    // TODO Need valid test cases once there is a stage after auth.
}

int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testJsonAuthInsecure),
        {0, ""}
    };
    return Test_util::runner("logjamd::Stage_json_adapt", tests);
}

