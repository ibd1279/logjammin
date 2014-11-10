/*!
 \file test/logjamd/Stage_preTest.cpp
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
#include "logjamd/Stage_pre.h"
#include "logjamd/constants.h"
#include "logjamd/mock_server.h"
#include <memory>
#include <sstream>

#include "test/logjamd/Stage_preTest_driver.h"

void testBSON()
{
    // Create the mock request.
    Mock_env env;
    env.swimmer->sink() << "BSON\n";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    
    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr);
    TEST_ASSERT(next_stage->name().compare("Authentication") == 0);
}

void testHTTP()
{
    // Create the mock request.
    Mock_env env;
    env.swimmer->sink() << "GET /print('Hello') HTTP/1.0";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr);
    TEST_ASSERT(next_stage->name().compare("HTTP-Adapter") == 0);
}

void testUnknown()
{
    // Create the mock request.
    Mock_env env;
    env.swimmer->sink() << "rtmp ";

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_pre());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));

    // Test the result.
    std::string expected("{\"message\":\"Unknown mode: rtmp\", \"stage\":\"Pre-connection\", \"success\":0}");
    std::ostringstream oss;
    oss << env.swimmer->source().rdbuf();
    TEST_ASSERT(oss.str().compare(expected) == 0);
    TEST_ASSERT(next_stage == NULL);
}

int main(int argc, char** argv)
{
    Mock_server_init ctx;
    return Test_util::runner("logjamd::Stage_pre", tests);
}

