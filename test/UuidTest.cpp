/*
 * File:   UuidTest.cpp
 * Author: jwatson
 *
 * Created on May 7, 2011, 11:42:53 PM
 */

#include "testhelper.h"
#include "lj/Uuid.h"
#include "test/UuidTest_driver.h"

void testStr()
{
    lj::Uuid uuid = lj::Uuid::k_nil;
    std::string result = uuid.str();
    TEST_ASSERT(result.compare("{00000000-0000-0000-0000-000000000000}/0") == 0);

    uuid = lj::Uuid{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    result = uuid.str();
    TEST_ASSERT(result.compare("{00010203-0405-0607-0809-0a0b0c0d0e0f}/283686952329330") == 0);

    uuid = lj::Uuid{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    result = uuid.str();
    TEST_ASSERT(result.compare("{100f0e0d-0c0b-0a09-0807-060504030201}/1157159078456959122") == 0);

    lj::Uuid uuid2("{100f0e0d-0c0b-0a09-0807-060504030201}");
    TEST_ASSERT(uuid2 == uuid);

    result = static_cast<std::string>(uuid);
    lj::Uuid uuid3(result);
    TEST_ASSERT(uuid3 == uuid);
}

void testRandom()
{
    const uint64_t id = 0x1121314151617181ULL;
    lj::Uuid first(id);
    for (int h = 0; h < 1000; ++h)
    {
        lj::Uuid after(id);
        TEST_ASSERT(first != after);
    }
}

void testData()
{
    size_t sz = 0;
    lj::Uuid uuid
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    };
    const uint8_t* result = uuid.data(&sz);
    TEST_ASSERT(sz == 16);
    for (size_t h = 0; h < sz; ++h)
    {
        TEST_ASSERT(result[h] == h);
    }
}

void testNamespaceDNS()
{
    const lj::Uuid dns("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    TEST_ASSERT(lj::Uuid::k_ns_dns == dns);
}

void testNamespaceURL()
{
    const lj::Uuid url("6ba7b811-9dad-11d1-80b4-00c04fd430c8");
    TEST_ASSERT(lj::Uuid::k_ns_url == url);
}

void testVersion5()
{
    const std::string name1("www.example.org");
    lj::Uuid result1(lj::Uuid::k_ns_dns, name1.c_str(), name1.size());
    TEST_ASSERT(lj::Uuid("74738ff5-5367-5958-9aee-98fffdcd1876") == result1);

    const std::string name2("python.org");
    lj::Uuid result2(lj::Uuid::k_ns_dns, name2.c_str(), name2.size());
    TEST_ASSERT(lj::Uuid("886313e1-3b8a-5372-9b90-0c9aee199e5d") == result2);

    const std::string name3("http://www.ietf.org/rfc/rfc4122.txt");
    lj::Uuid result3(lj::Uuid::k_ns_url, name3.c_str(), name3.size());
    TEST_ASSERT(lj::Uuid("d0690b3c-b29d-52e7-81b0-d573b503f2d4") == result3);

    lj::Uuid result4(lj::Uuid::k_ns_dns, name3.c_str(), name3.size());
    TEST_ASSERT(result3 != result4);
}

void testInteger()
{
    uint64_t expected = 0xFFFFFFFFFFFFFFFFULL;
    lj::Uuid foo(expected);
    uint64_t result = static_cast<uint64_t>(foo);
    TEST_ASSERT(expected == result);
    foo = lj::Uuid(static_cast<uint64_t>(0));
    result = static_cast<uint64_t>(foo);
    TEST_ASSERT(0 == result);
}

void testLessThan()
{
    lj::Uuid low(100000);
    lj::Uuid high(900000);
    lj::Uuid dup(low);
    TEST_ASSERT(low < high);
    TEST_ASSERT(!(low > high));
    TEST_ASSERT(!(high < low));
    TEST_ASSERT(high > low);
    TEST_ASSERT(!(low > low));
    TEST_ASSERT(!(low < low));
    TEST_ASSERT(!(low > dup));
    TEST_ASSERT(!(low < dup));
}

void testLessThanOrEqual()
{
    lj::Uuid low(100000);
    lj::Uuid high(900000);
    lj::Uuid dup(low);
    TEST_ASSERT(low <= high);
    TEST_ASSERT(!(low >= high));
    TEST_ASSERT(!(high <= low));
    TEST_ASSERT(high >= low);
    TEST_ASSERT((low >= low));
    TEST_ASSERT((low <= low));
    TEST_ASSERT((low >= dup));
    TEST_ASSERT((low <= dup));
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::UuidTest", tests);
}

