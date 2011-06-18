/* 
 * File:   Log_test.cpp
 * Author: jwatson
 *
 * Created on May 13, 2011, 7:48:07 PM
 */

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include "lj/Log.h"
#include "testhelper.h"

void testWrite_disable()
{
    std::ostringstream buffer;
    lj::Log logger(lj::Log::Level::k_warning, &buffer);
    logger.disable();
    logger.log("%s", "test string");
    logger("%s") << "test string" << lj::Log::end;
    
    logger.enable();
    logger.log("searching logs for [%s]", "test string");
    logger("searching logs for [%s]") << "test string" << lj::Log::end;

    std::string expected = "[WARNING]     searching logs for [test string]\n[WARNING]     searching logs for [test string]\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_string()
{
    std::ostringstream buffer;
    lj::Log logger(lj::Log::Level::k_error, &buffer);
    logger.log("%s", "test string");
    logger("%s") << "test string" << lj::Log::end;
    logger.log("searching logs for [%s]", "test string");
    logger("searching logs for [%s]") << "test string" << lj::Log::end;

    std::string expected = "[ERROR]       test string\n[ERROR]       test string\n[ERROR]       searching logs for [test string]\n[ERROR]       searching logs for [test string]\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_signed_int()
{
    std::ostringstream buffer;
    lj::Log logger(lj::Log::Level::k_emergency, &buffer);
    logger.log("%hd", 0xFFFF);
    logger("%hd") << (int16_t)0xFFFF << lj::Log::end;
    logger.log("%lld", 0xAABB0011AABB0011LL);
    logger("%lld") << (int64_t)0xAABB0011AABB0011LL << lj::Log::end;
    std::string expected = "[EMERGENCY]   -1\n[EMERGENCY]   -1\n[EMERGENCY]   -6144317190738083823\n[EMERGENCY]   -6144317190738083823\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_unsigned_int()
{
    std::ostringstream buffer;
    lj::Log logger(lj::Log::Level::k_emergency, &buffer);
    logger.log("%hu", 65535);
    logger("%hu") << (uint16_t)65535 << lj::Log::end;
    logger.log("%llu", 0xAABB0011AABB0011ULL);
    logger("%llu") << (uint64_t)0xAABB0011AABB0011ULL << lj::Log::end;
    std::string expected = "[EMERGENCY]   65535\n[EMERGENCY]   65535\n[EMERGENCY]   12302426882971467793\n[EMERGENCY]   12302426882971467793\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_bool()
{
    std::ostringstream buffer;
    lj::Log logger(lj::Log::Level::k_critical, &buffer);
    logger.log("%d", false);
    logger("%d") << false << lj::Log::end;
    logger.log("%d", true);
    logger("%d") << true << lj::Log::end;
    logger("%s") << false << lj::Log::end;
    logger("%s") << true << lj::Log::end;
    std::string expected = "[CRITICAL]    0\n[CRITICAL]    0\n[CRITICAL]    1\n[CRITICAL]    1\n[CRITICAL]    false\n[CRITICAL]    true\n";
    TEST_ASSERT(expected.compare(buffer.str()) == 0);
}

void testWrite_pointer()
{
    std::ostringstream buffer;
    lj::Log logger(lj::Log::Level::k_alert, &buffer);
    logger.log("%p", &buffer);
    logger("%p") << &buffer << lj::Log::end;
    char expected[1024];
    sprintf(expected, "[ALERT]       %p\n[ALERT]       %p\n", &buffer, &buffer);
    TEST_ASSERT(std::string(expected).compare(buffer.str()) == 0);
}

int main(int argc, char** argv)
{
    const Test_entry tests[] = {
        PREPARE_TEST(testWrite_string),
        PREPARE_TEST(testWrite_signed_int),
        PREPARE_TEST(testWrite_unsigned_int),
        PREPARE_TEST(testWrite_bool),
        PREPARE_TEST(testWrite_pointer),
        {0, ""}
    };
    return Test_util::runner("lj::LogTest", tests);
}

