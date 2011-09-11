/* 
 * File:   Stage_json_adaptTest.cpp
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Stage_json_adapt.h"
#include "logjamd/Stage_pre.h"
#include "logjamd/constants.h"

#include "test/logjamd/Stage_json_adaptTest_driver.h"
void testJsonAuthInsecure()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "json\n";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();

    // Test the next stage.
    TEST_ASSERT(next_stage != NULL);
    logjamd::Stage_json_adapt* adapter =
            dynamic_cast<logjamd::Stage_json_adapt*>(next_stage);
    TEST_ASSERT(adapter != NULL);

    // Test the result. JSON does an auto-login.
    TEST_ASSERT(adapter->faux_connection().user() != NULL);
    TEST_ASSERT(adapter->faux_connection().user()->id() == logjamd::k_user_id_json);
    TEST_ASSERT(adapter->faux_connection().user()->login().compare(logjamd::k_user_login_json) == 0);

    // Clean up the adapter stage.
    delete next_stage;
    
}

int main(int argc, char** argv)
{
    return Test_util::runner("logjamd::Stage_json_adapt", tests);
}

