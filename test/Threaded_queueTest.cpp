/*!
 \file test/ArgsTest.cpp
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
#include "lj/Threaded_queue.h"
#include "test/Threaded_queueTest_driver.h"

#include "lj/Stopclock.h"
#include "lj/Thread.h"
#include <chrono>
#include <map>
#include <thread>
#include <vector>

void testSingleThreadSingleItem()
{
    lj::Threaded_queue<int> obj;

    const int expected = 100;
    obj.push(expected);
    int result = obj.pop();

    TEST_ASSERT(result == expected);
}

void testSingleThreadMultipleItems()
{
    lj::Threaded_queue<int> obj;
    const std::vector<int> expected { 100, 200, 300, 400 };
    std::vector<int> result(4);

    obj.push(expected[0]);
    obj.push(expected[1]);
    obj.push(expected[2]);

    result[0] = obj.pop();

    obj.push(expected[3]);

    result[1] = obj.pop();
    result[2] = obj.pop();
    result[3] = obj.pop();

    for (int h = 0; h < 4; ++h)
    {
        TEST_ASSERT(result[h] == expected[h]);
    }
}

typedef lj::Thread::Lambda_work<std::function<void()>, std::function<void()> > Test_work;

void testTwoThreadsSingleItem()
{
    lj::Threaded_queue<int> obj;
    const int expected = 200;
    int result = 0;

    Test_work* producer = new Test_work([expected, &obj](){
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            obj.push(expected);}, [](){});
    Test_work* consumer = new Test_work([&result, &obj](){
                lj::Stopclock clock;
                result = obj.pop();
                lj::log::format<lj::Alert>("Took %llu nanoseconds to pop").end(clock.elapsed());},
            [&expected, &result](){});

    lj::Thread prod, cons;
    prod.run(producer);
    cons.run(consumer);
    prod.join();
    cons.join();
    TEST_ASSERT(result == expected);
}

void testFiveProducersSingleConsumer()
{
    lj::Threaded_queue<int> obj;
    const std::list<int> input {
        1, 2, 1, 2, 3, 4, 5, 1, 3, 2, 3, 4, 1, 5, 6, 9, 1, 2, 3, 7, 8, 3, 1,
        1, 2, 3, 4, 5, 3, 9, 2, 1, 6, 8, 3, 7, 2, 3, 8, 9, 1, 1, 6, 8, 2, 0};
    std::vector<lj::Thread> producing_threads(10);
    std::map<int, int> expected, results;
    for (int value : input)
    {
        expected[value]++;
    }
    for (std::map<int, int>::value_type& p : expected)
    {
        p.second = p.second * producing_threads.size();
    }

    Test_work producer([input, &obj](){
                for (int value : input)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    obj.push(value);
                }
            }, [](){});
    Test_work* consumer = new Test_work([&results, &obj, &producing_threads, input](){
                lj::Stopclock clock;
                for (int h = producing_threads.size() * input.size(); h > 0; --h)
                {
                    int value = obj.pop();
                    ++(results[value]);
                }
                lj::log::format<lj::Alert>("Took %llu nanoseconds to pop").end(clock.elapsed());
            },
            [](){});

    lj::Thread cons;
    cons.run(consumer);
    for (lj::Thread& t : producing_threads)
    {
        t.run(new Test_work(producer));
    }

    cons.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (auto p : expected)
    {
        TEST_ASSERT(results[p.first] == p.second);
    }
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::Threaded_queue", tests);
}

