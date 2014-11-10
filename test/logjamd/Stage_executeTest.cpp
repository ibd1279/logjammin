/*!
 \file test/logjamd/Stage_executeTest.cpp
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
#include "logjamd/Stage_execute.h"
#include "logjamd/constants.h"

#include "test/logjamd/Stage_executeTest_driver.h"

void testBasicCommands()
{
    // Set up testing environment.
    Mock_env env;

    // Test command
    lj::bson::Node request;
    request.set_child("command", lj::bson::new_string( "print ('Hello LJ')\n\
print ('testing', 'foobar', Uuid:new('{444df00e-95ce-4dd6-8f1c-6dc8b96f92d9}'))\n\
print (Document:new())\n\
print (Uuid:new())"));

    // Put the data on the pipe.
    env.swimmer->sink() << request;

    // perform the stage.
    std::unique_ptr<logjam::Stage> next_stage(
            new logjamd::Stage_execute());
    next_stage = logjam::safe_execute_stage(next_stage, *(env.swimmer));
    lj::bson::Node response;
    env.swimmer->source() >> response;

    // Test the next stage.
    TEST_ASSERT(next_stage != nullptr);

    // Test the output.
    std::cout << lj::bson::as_string(response) << std::endl;
    TEST_ASSERT(lj::bson::as_string(response["output/0"]).compare("Hello LJ") == 0);
    TEST_ASSERT(lj::bson::as_string(response["output/1"]).compare("testing\tfoobar\t{444df00e-95ce-4dd6-8f1c-6dc8b96f92d9}") == 0);
}

int main(int argc, char** argv)
{
    return Test_util::runner("logjamd::Stage_execute", tests);
}

