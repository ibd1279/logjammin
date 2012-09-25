/*
 * File:   ThreadTest.cpp
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

