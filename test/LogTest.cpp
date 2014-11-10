/*!
 \file test/LogTest.cpp
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
#include "lj/Exception.h"
#include "lj/Log.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <thread>
#include "test/LogTest_driver.h"

void testWrite_disable()
{
    lj::log::enable<lj::Debug > ();
    lj::log::Logger& logger2 = lj::log::format<lj::Debug > ("foo");
    lj::log::Logger_clog* ptr2 = dynamic_cast<lj::log::Logger_clog*>(&logger2);
    TEST_ASSERT(ptr2 != NULL);
    logger2.end();

    lj::log::disable<lj::Debug > ();
    lj::log::Logger& logger1 = lj::log::format<lj::Debug > ("foo");
    lj::log::Logger_clog* ptr1 = dynamic_cast<lj::log::Logger_clog*>(&logger1);
    TEST_ASSERT(ptr1 == NULL);
    logger1.end();
}

void testWrite_string()
{
    std::ostringstream buffer;
    lj::log::Logger& logger1 =
            *(new lj::log::Logger_stream("ERROR", "%s", &buffer));
    logger1.end("test string");

    std::string expected = "[ERROR] test string\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_signed_int()
{
    std::ostringstream buffer;
    lj::log::Logger& logger1 =
            *(new lj::log::Logger_stream("ERROR", "%hd %lld", &buffer));
    logger1.end((int16_t)0xFFFF, (int64_t)0xAABB0011AABB0011LL);

    std::string expected = "[ERROR] -1 -6144317190738083823\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_unsigned_int()
{
    std::ostringstream buffer;
    lj::log::Logger& logger1 =
            *(new lj::log::Logger_stream("ERROR", "%hu %llu", &buffer));
    logger1.end((uint16_t)65535, (uint64_t)0xAABB0011AABB0011ULL);

    std::string expected = "[ERROR] 65535 12302426882971467793\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_bool()
{
    std::ostringstream buffer;
    lj::log::Logger& logger1 =
            *(new lj::log::Logger_stream("ERROR", "%d %d %s %s", &buffer));
    logger1.end(false, true, false, true);

    std::string expected = "[ERROR] 0 1 false true\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_pointer()
{
    std::ostringstream buffer;
    lj::log::Logger& logger1 =
            *(new lj::log::Logger_stream("ERROR", "%p %p", &buffer));
    logger1.end(&buffer, &logger1);

    char expected[1024];
    sprintf(expected, "[ERROR] %p %p\n", &buffer, &logger1);
    TEST_ASSERT(std::string(expected).compare(buffer.str()) == 0);
}

void testWrite_exception()
{
    std::ostringstream buffer;
    lj::log::Logger& logger1 =
            *(new lj::log::Logger_stream("ERROR", "%s", &buffer));
    logger1.end(LJ__Exception("foo bar"));

    char expected[1024];
    sprintf(expected, "[ERROR] %s Exception: %s - foo bar\n", __FILE__, __FUNCTION__);
    TEST_ASSERT(std::string(expected).compare(buffer.str()) == 0);
}

void testCatch_and_log()
{
    std::ostringstream buffer;
    lj::log::Logger& logger1 =
            *(new lj::log::Logger_stream("ERROR", "%s: %s", &buffer));

    lj::log::attempt(logger1, [] ()
    {
        throw LJ__Exception("random fail");
    });
    std::string expected("[ERROR] Unhandled Exception: ../test/LogTest.cpp Exception: operator() - random fail\n");
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::LogTest", tests);
}

