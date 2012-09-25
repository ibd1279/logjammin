#pragma once
/*!
 \file test/testhelper.h
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

#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h>

struct Test_failure
{

    Test_failure(const std::string& msg, const std::string& expr,
            const std::string& file, const std::string& func, int line) :
            _msg(msg), _expr(expr), _file(file), _func(func), _line(line)
    {
    }

    std::string details(const std::string& suite_name, const std::string& test_name, float elapsed) const
    {
        std::ostringstream oss;
        oss << "%TEST_FAILED% time=";
        oss << std::setiosflags(std::ios::fixed) << std::setprecision(4) << elapsed;
        oss << " testname=" << test_name;
        oss << " (" << suite_name << ") message=" << _msg;
        oss << " (" << _expr << ") in " << _func << " at " << _file << ":" << _line;
        return oss.str();
    }
    std::string _msg, _expr, _file, _func;
    int _line;
};

struct Test_entry
{
    void (*f)();
    std::string n;
};

#define PREPARE_TEST(func) (Test_entry{&func, #func})

struct Test_util
{

    static void fail_if(bool expr, const Test_failure& fail_msg)
    {
        if (expr)
        {
            throw fail_msg;
        }
    }

    static unsigned long long elapsed(const struct timeval& start)
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        return (((now.tv_sec - start.tv_sec) * 1000000ULL) +
                (now.tv_usec - start.tv_usec));
    }

    static int runner(const std::string& suite_name, const Test_entry* tests)
    {
        int failures = EXIT_SUCCESS;
        std::cout << "%SUITE_STARTING% " << suite_name << std::endl;

        struct timeval start;
        gettimeofday(&start, NULL);
        std::cout << "%SUITE_STARTED%" << std::endl;

        while (tests->f != NULL)
        {
            struct timeval test_start;
            gettimeofday(&test_start, NULL);
            std::cout << "%TEST_STARTED% " << tests->n << " (" << suite_name << ")" << std::endl;
            try
            {
                (*(tests->f))();
            }
            catch (const Test_failure& failure)
            {
                std::cout << failure.details(suite_name, tests->n, (elapsed(test_start) / 1000000.0f)) << std::endl;
                failures++;
            }
            catch (const std::exception& ex)
            {
                std::cout << Test_failure(std::string(ex.what()), "unknown", "unknown", "unknown", -1).details(suite_name, tests->n, (elapsed(test_start) / 1000000.0f)) << std::endl;
                failures++;
            }
            catch (const std::exception* ex)
            {
                std::cout << Test_failure(std::string(ex->what()), "unknown", "unknown", "unknown", -1).details(suite_name, tests->n, (elapsed(test_start) / 1000000.0f)) << std::endl;
                failures++;
            }
            std::cout << "%TEST_FINISHED% time=";
            std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(4);
            std::cout << (elapsed(test_start) / 1000000.0f);
            std::cout << " " << tests->n << " (" << suite_name << ")" << std::endl;
            tests++;
        }

        std::cout << "%SUITE_FINISHED% time=";
        std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(4);
        std::cout << (elapsed(start) / 1000000.0f);
        std::cout << std::endl;

        return (failures);
    }
};

#define TEST_ASSERT_MSG(msg, expr) (Test_util::fail_if(!(expr), Test_failure(msg, #expr, __FILE__, __FUNCTION__, __LINE__)))
#define TEST_ASSERT(expr) TEST_ASSERT_MSG("Assert Failed", expr)
#define TEST_FAILED(msg) (Test_util::fail_if(true, Test_failure(msg, "<See Test>", __FILE__, __FUNCTION__, __LINE__)))
