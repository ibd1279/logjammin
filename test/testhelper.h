#pragma once
/* 
 * File:   testhelper.h
 * Author: jwatson
 *
 * Created on May 7, 2011, 11:44 PM
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
    Test_failure(const std::string& msg, const std::string& expr, const std::string& file,
    const std::string& func, int line) : _msg(msg), _expr(expr), _file(file), _func(func), _line(line)
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
        if(expr)
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
