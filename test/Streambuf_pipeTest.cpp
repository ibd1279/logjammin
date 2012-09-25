/*
 * File:   Streambuf_pipeTest.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Streambuf_pipe.h"

#include <istream>
#include <ostream>
#include <fstream>

#include "test/Streambuf_pipeTest_driver.h"

#define MEM_LENGTH (512*1024)

char* random_stream()
{
    std::ifstream rand("/dev/urandom");
    char* ptr = new char[MEM_LENGTH];
    int len = 0;
    while (len < MEM_LENGTH)
    {
        rand.read(ptr + len, MEM_LENGTH - len);
        len += rand.gcount();
    }
    return ptr;
}

void testSerial()
{
    lj::Streambuf_pipe pipe;
    std::iostream stream(&pipe);
    char* rand_array1 = random_stream();
    char* rand_array2 = random_stream();

    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        pipe.sink().put(rand_array1[h]);
        stream.put(rand_array2[h]);
    }

    char c;
    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        c = stream.get();
        TEST_ASSERT(c == rand_array1[h]);
        c = pipe.source().get();
        TEST_ASSERT(c == rand_array2[h]);
    }
}

void testMixed()
{
    lj::Streambuf_pipe pipe;
    std::iostream stream(&pipe);
    char* rand_array1 = random_stream();
    char* rand_array2 = random_stream();
    char* rand_array3 = random_stream();
    char c;
    int left_in = 0;
    int left_out = 0;
    int right_in = 0;
    int right_out = 0;

    for (int h = 1; h < MEM_LENGTH; ++h)
    {
        switch (rand_array3[h] % 4)
        {
            case 0:
                if (left_out < right_in)
                {
                    c = pipe.source().get();
                    TEST_ASSERT(c == rand_array2[left_out++]);
                    break;
                }
            case 1:
                pipe.sink().put(rand_array1[left_in++]);
                break;
            case 2:
                if (right_out < left_in)
                {
                    c = stream.get();
                    TEST_ASSERT(c == rand_array1[right_out++]);
                    break;
                }
            default:
                stream.put(rand_array2[right_in++]);
                break;
        }
    }
}

int main(int argc, char** argv)
{

    const Test_entry tests[] = {
        PREPARE_TEST(testSerial),
        PREPARE_TEST(testMixed),
        {
            0, ""}
    };
    return Test_util::runner("lj::Streambuf_pipe", tests);
}

