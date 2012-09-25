/*
 * File:   StopclockTest.cpp
 * Author: jwatson
 *
 * Created on May 18, 2011, 10:02:49 PM
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

