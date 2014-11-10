/*!
 \file test/Streambuf_pipeTest.cpp
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

