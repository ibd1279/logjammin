/* 
 * File:   lj_base64.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Uuid.h"
#include "lj/Thread.h"
#include "lj/Stopclock.h"

#include <vector>

class TestWork : public lj::Work
{
public:
    TestWork() : stop_(false)
    {
    }
    virtual void run()
    {
        lj::Uuid* id = new lj::Uuid();
        for (int h = 0; h < 500 && !stop_; ++h)
        {
            delete id;
            id = new lj::Uuid();
        }
        delete id;
    }
    virtual void* call()
    {
        lj::Stopclock clock;
        lj::Uuid* id = new lj::Uuid();
        for (int h = 0; h < 500 && !stop_; ++h)
        {
            delete id;
            id = new lj::Uuid();
        }
        std::cerr << std::setiosflags(std::ios::fixed) << std::setprecision(4);
        std::cerr << (clock.elapsed() / 1000000.0f) << std::endl;
        return id;
    }
    virtual void abort()
    {
        stop_ = true;
    }
private:
    volatile bool stop_;
};

void testRun()
{
    lj::Thread t;
    lj::Work* w = new TestWork();

    t.run(w);
    TEST_ASSERT(t.running() == true);
}

void testCall()
{
    lj::Thread t;
    lj::Work* w = new TestWork();

    lj::Future* f = t.call(w);
    TEST_ASSERT(t.running() == true);
    lj::Uuid* id = f->result<lj::Uuid>();
    TEST_ASSERT(id != NULL);
    TEST_ASSERT(*id != lj::Uuid::k_nil);
}

void testAbort()
{
    lj::Thread t;
    lj::Work* w = new TestWork();

    t.run(w);
    TEST_ASSERT(t.running() == true);
    t.abort();
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
    catch(std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    try
    {
        t.call(w);
        TEST_FAILED("Should not be able to call run while running.");
    }
    catch(std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

void testMultipleThreads()
{
    std::vector<lj::Thread*> threads;
    std::vector<lj::Future*> futures;

    for (int h = 0; h < 10; ++h)
    {
        lj::Thread* t = new lj::Thread();
        futures.push_back(t->call(new TestWork()));
        threads.push_back(t);
    }

    for (int h = 0; h < 10; ++h)
    {
        TEST_ASSERT(*futures[h]->result<lj::Uuid>() != lj::Uuid::k_nil);
        delete futures[h];
        delete threads[h];
    }
}

int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testRun),
        PREPARE_TEST(testCall),
        PREPARE_TEST(testAbort),
        PREPARE_TEST(testDualRun),
        PREPARE_TEST(testMultipleThreads),
        {0, ""}
    };
    return Test_util::runner("lj::Thread", tests);
}

