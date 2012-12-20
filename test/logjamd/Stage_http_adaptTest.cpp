/* 
 * File:   Stage_http_adaptTest.cpp
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Stage_http_adapt.h"
#include "logjamd/Stage_pre.h"
#include "logjamd/constants.h"

#include "test/logjamd/Stage_http_adaptTest_driver.h"
void testHttpGetInsecure()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "get /print('Hello, world') HTTP/1.0\r\n";
    env.request() << "Host: localhost:12345\r\n";
    env.request() << "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/536.26.17 (KHTML, like Gecko) Version/6.0.2 Safari/536.26.17\r\n";
    env.request() << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    env.request() << "Cache-Control: max-age=0\r\n";
    env.request() << "Accept-Language: en-us\r\n";
    env.request() << "Accept-Encoding: gzip, deflate\r\n";
    env.request() << "Connection: keep-alive\r\n";
    env.request() << "\r\n";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();

    // Test the next stage. -- this is really a test of pre.
    TEST_ASSERT(next_stage != NULL);
    logjamd::Stage_http_adapt* adapter =
            dynamic_cast<logjamd::Stage_http_adapt*>(next_stage);
    TEST_ASSERT(adapter != NULL);

    // Test the http adapt logic.
    next_stage->logic();
    
    // Clean up the adapter stage.
    delete next_stage;
}

void testHttpPostInsecure()
{
    // Create the mock request.
    Mock_environment env;
    env.request() << "post / HTTP/1.0\r\n";
    env.request() << "Host: localhost:12345\r\n";
    env.request() << "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/536.26.17 (KHTML, like Gecko) Version/6.0.2 Safari/536.26.17\r\n";
    env.request() << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    env.request() << "Cache-Control: max-age=0\r\n";
    env.request() << "Accept-Language: en-us\r\n";
    env.request() << "Accept-Encoding: gzip, deflate\r\n";
    env.request() << "Connection: keep-alive\r\n";
    env.request() << "Content-Length: 21\r\n";
    env.request() << "\r\n";
    env.request() << "print('Hello, world')";

    // perform the stage.
    logjamd::Stage_pre stage(env.connection());
    logjamd::Stage* next_stage = stage.logic();

    // Test the next stage. -- this is really a test of pre.
    TEST_ASSERT(next_stage != NULL);
    logjamd::Stage_http_adapt* adapter =
            dynamic_cast<logjamd::Stage_http_adapt*>(next_stage);
    TEST_ASSERT(adapter != NULL);

    // Test the http adapt logic.
    next_stage->logic();
    
    // Clean up the adapter stage.
    delete next_stage;
}

int main(int argc, char** argv)
{
    Mock_server_init ctx;
    return Test_util::runner("logjamd::Stage_http_adapt", tests);
}

