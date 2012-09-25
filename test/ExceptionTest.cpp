/*
 * File:   ExceptionTest.cpp
 * Author: jwatson
 *
 * Created on May 10, 2011, 11:46:43 PM
 */

#include "testhelper.h"
#include "lj/Exception.h"
#include "test/ExceptionTest_driver.h"

void testException()
{
    lj::Exception exception("Test System", "Sample Message");

    TEST_ASSERT(exception.str().compare("Test System Exception: Sample Message") == 0);
    TEST_ASSERT(((std::string)exception).compare("Test System Exception: Sample Message") == 0);
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::ExceptionTest", tests);
}

