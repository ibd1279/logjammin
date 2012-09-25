/*!
 \file test/ThreadTest.cpp
 Copyright (c) 2010, Jason Watson
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

#include <vector>
#include "testhelper.h"
#include "lj/Uuid.h"
#include "lj/Stopclock.h"
#include "test/ThreadTest_driver.h"
#include "lj/Thread.h"

class TestWork : public lj::Work
{
public:

    TestWork() : lj::Work(), state_("Created")
    {
    }

    virtual void run()
    {
        state_ = "Running";
        lj::Uuid* id = new lj::Uuid();
        for (int h = 0; h < 100; ++h)
        {
            delete id;
            id = new lj::Uuid();
        }
        delete id;
    }

    virtual void cleanup()
    {
        state_ = "Cleanup";
    }
    std::string state_;
};

class TestExceptionWork : public lj::Work
{
public:

    TestExceptionWork() : lj::Work()
    {
    }

    virtual void run()
    {
        throw LJ__Exception("Run Exception");
    }

    virtual void cleanup()
    {
        throw LJ__Exception("Cleanup Exception");
    }
};

void testRun1()
{
    lj::Thread t;
    lj::Work* w = new TestWork();

    t.run(w);
    TEST_ASSERT(t.running() == true);
    t.join();
    TEST_ASSERT(t.running() == false);
}

void testRun2()
{
    int x = 200;
    lj::Thread t;

    auto func = [&x](){for (x = 0; x < 100; ++x);};

    lj::Thread::Lambda_work < std::function<void()>, std::function<void()> >* w =
            new lj::Thread::Lambda_work < std::function<void()>, std::function<void()> >(func,
            [&x]()
    {
        lj::log::format<lj::Alert > ("outputting %d").end(x);});
    t.run(w);
    TEST_ASSERT(t.running() == true);
    t.join();
    TEST_ASSERT(t.running() == false);
}

void testRun3()
{
    int x = 200;
    lj::Thread t;

    t.run([&x]()
    {

        for (x = 0; x < 100; ++x); },
    [&x]()
    {
        lj::log::format<lj::Alert > ("outputting %d").end(x); });
    TEST_ASSERT(t.running() == true);
    t.join();
    TEST_ASSERT(t.running() == false);
    TEST_ASSERT(x == 100);
}

void testDualRun()
{
    lj::Thread t;
    lj::Work* w = new TestWork();

    t.run(w);
    TEST_ASSERT(t.running() == true);
    try
    {
        t.run(w);
        TEST_FAILED("Should not be able to call run while running.");
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    t.join();
    TEST_ASSERT(t.running() == false);
}

void testRunException()
{
    lj::Thread t;
    lj::Work* w = new TestExceptionWork();

    t.run(w);
    t.join();
    TEST_ASSERT(t.running() == false);
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::Thread", tests);
}

