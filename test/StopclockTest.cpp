/*!
 \file test/StopclockTest.cpp
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

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include "lj/Stopclock.h"
#include "lj/Uuid.h"
#include "testhelper.h"
#include "test/StopclockTest_driver.h"

void testNormal()
{
    uint64_t h = 0;

    lj::Stopclock stopclock;
    while (stopclock.elapsed() <= 100000ULL)
    {
        // do nothing, just waste time.
        ++h;
    }
    uint64_t lap1 = stopclock.stop();
    TEST_ASSERT(lap1 > 100000ULL);
}

void testStop()
{
    uint64_t h = 0;

    lj::Stopclock stopclock;
    while (stopclock.elapsed() <= 10000ULL)
    {
        // do nothing, just waste time.
        ++h;
    }
    uint64_t lap1 = stopclock.stop();
    TEST_ASSERT(lap1 > 10000ULL);

    // Ensure stop does stop the clock.
    h = 0;
    while (h <= 100ULL)
    {
        ++h;
        lj::Uuid uuid;
        uuid.str();
        // do nothing, just waste time.
    }
    uint64_t lap2 = stopclock.elapsed();
    TEST_ASSERT(lap2 == lap1);
}

void testRestart()
{
    uint64_t h = 0;

    lj::Stopclock stopclock;
    while (stopclock.elapsed() <= 100000ULL)
    {
        // do nothing, just waste time.
        ++h;
    }
    uint64_t lap1 = stopclock.stop();
    TEST_ASSERT(lap1 > 100000ULL);

    // test restarting.
    stopclock.start();
    while (stopclock.elapsed() <= 10000ULL)
    {
        // do nothing, just waste time.
        ++h;
    }
    uint64_t lap2 = static_cast<uint64_t>(stopclock);
    TEST_ASSERT(lap2 < lap1);
    TEST_ASSERT(lap2 > 10000ULL);
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::StopclockTest", tests);
}

