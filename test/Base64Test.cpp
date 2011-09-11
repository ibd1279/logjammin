/* 
 * File:   Base64Test.cpp
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Base64.h"
#include "test/Base64Test_driver.h"

void testBase64()
{
    size_t sz;
    const uint8_t data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A, 0x79, 0x88, 0x97, 0xA6, 0xB5, 0xC4, 0xD3, 0xE2, 0xF1};
    std::string encoded = lj::base64_encode(data, 31);
    uint8_t* decoded = lj::base64_decode(encoded, &sz);
    TEST_ASSERT(sz == 31);
    TEST_ASSERT(data != decoded);
    for(int h = 0; h < 31; ++h)
    {
        TEST_ASSERT(data[h] == decoded[h]);
    }
    delete[](decoded);
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::Base64", tests);
}

