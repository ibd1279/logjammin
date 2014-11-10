/*!
 \file test/logjamd/Stage_http_adaptTest.cpp
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

#include "testhelper.h"
#include "lj/Bson.h"
#include "logjamd/mock_server.h"
#include "logjamd/Stage_http_adapt.h"
#include "logjamd/Stage_pre.h"
#include "logjamd/constants.h"

#include "test/logjamd/Stage_http_adaptTest_driver.h"
void testHttpGetInsecure()
{
    // Set up testing environment.
    Mock_env env;

    // Create the mock request.
    env.swimmer->sink() << "get /print('Hello, world') HTTP/1.0\r\n";
    env.swimmer->sink() << "Host: localhost:12345\r\n";
    env.swimmer->sink() << "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/536.26.17 (KHTML, like Gecko) Version/6.0.2 Safari/536.26.17\r\n";
    env.swimmer->sink() << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    env.swimmer->sink() << "Cache-Control: max-age=0\r\n";
    env.swimmer->sink() << "Accept-Language: en-us\r\n";
    env.swimmer->sink() << "Accept-Encoding: gzip, deflate\r\n";
    env.swimmer->sink() << "Connection: keep-alive\r\n";
    env.swimmer->sink() << "\r\n";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Test the next stage. -- this is really a test of pre.
    TEST_ASSERT(next_stage != NULL);
    logjamd::Stage_http_adapt* adapter =
            dynamic_cast<logjamd::Stage_http_adapt*>(next_stage.get());
    TEST_ASSERT(adapter != NULL);

    // perform the HTTP stage.
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Because the output is http data, we have to process it
    // back into a bson node.
    std::ostringstream oss;
    oss << env.swimmer->source().rdbuf();
    std::string result_string(oss.str());
    result_string.erase(0,
            result_string.find("\r\n\r\n"));
    std::unique_ptr<lj::bson::Node> result_ptr(
            lj::bson::parse_json(result_string));
    lj::bson::Node& result(*result_ptr);

    // Validate the result.
    std::string expected_stage("Execution");
    std::string expected_output("[\"0\":\"Hello, world\"]");
    TEST_ASSERT(0 == expected_stage.compare(lj::bson::as_string(result["stage"])));
    TEST_ASSERT(0 == expected_output.compare(lj::bson::as_string(result["output"])));
    TEST_ASSERT(lj::bson::as_boolean(result["success"]));
}

void testHttpPostInsecure()
{
    // Set up testing environment.
    Mock_env env;

    // Create the mock request.
    env.swimmer->sink() << "post / HTTP/1.0\r\n";
    env.swimmer->sink() << "Host: localhost:12345\r\n";
    env.swimmer->sink() << "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/536.26.17 (KHTML, like Gecko) Version/6.0.2 Safari/536.26.17\r\n";
    env.swimmer->sink() << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    env.swimmer->sink() << "Cache-Control: max-age=0\r\n";
    env.swimmer->sink() << "Accept-Language: en-us\r\n";
    env.swimmer->sink() << "Accept-Encoding: gzip, deflate\r\n";
    env.swimmer->sink() << "Connection: keep-alive\r\n";
    env.swimmer->sink() << "Content-Length: 25\r\n";
    env.swimmer->sink() << "\r\n";
    env.swimmer->sink() << "cmd=print('Hello,+world')";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Test the next stage. -- this is really a test of pre.
    TEST_ASSERT(next_stage != NULL);
    logjamd::Stage_http_adapt* adapter =
            dynamic_cast<logjamd::Stage_http_adapt*>(next_stage.get());
    TEST_ASSERT(adapter != NULL);

    // perform the HTTP stage.
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Because the output is http data, we have to process it
    // back into a bson node.
    std::ostringstream oss;
    oss << env.swimmer->source().rdbuf();
    std::string result_string(oss.str());
    result_string.erase(0,
            result_string.find("\r\n\r\n"));
    std::unique_ptr<lj::bson::Node> result_ptr(
            lj::bson::parse_json(result_string));
    lj::bson::Node& result(*result_ptr);

    // Validate the result.
    std::cout << lj::bson::as_json_string(result) << std::endl;
    std::string expected_stage("Execution");
    std::string expected_output("[\"0\":\"Hello, world\"]");
    TEST_ASSERT(0 == expected_stage.compare(lj::bson::as_string(result["stage"])));
    TEST_ASSERT(0 == expected_output.compare(lj::bson::as_string(result["output"])));
    TEST_ASSERT(lj::bson::as_boolean(result["success"]));
}

int main(int argc, char** argv)
{
    Mock_server_init ctx;
    return Test_util::runner("logjamd::Stage_http_adapt", tests);
}

