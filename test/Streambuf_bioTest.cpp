/*!
 \file test/Streambuf_bioTest.cpp
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

#include "testhelper.h"
#include "lj/Streambuf_bio.h"

#include <istream>
#include <ostream>
#include <fstream>

#include "test/Streambuf_bioTest_driver.h"

#define MEM_LENGTH (1024*1024)

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

void testRead()
{
    char* rand_array = random_stream();
    BIO* mem = BIO_new_mem_buf(rand_array, MEM_LENGTH);
    lj::Streambuf_bio<char> buf(mem, 1024, 1);
    std::istream stream(&buf);
    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        int source;
        int dest;
        try
        {
            source = rand_array[h];
            dest = lj::Streambuf_bio<char>::traits_type::to_char_type(stream.get());
            TEST_ASSERT(source == dest);
        }
        catch (const Test_failure& ex)
        {
            std::cout << "unmatched set at " << h
                    << " for " << source << " = "
                    << dest << std::endl;
            throw ex;
        }
    }
}

void testWrite()
{
    char* rand_array = random_stream();
    BIO* mem = BIO_new(BIO_s_mem());
    lj::Streambuf_bio<char> buf(mem, 1, 1024);
    std::ostream stream(&buf);
    stream.write(rand_array, MEM_LENGTH);
    stream.flush();

    BUF_MEM* output_array;
    BIO_get_mem_ptr(mem, &output_array);
    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        int source;
        int dest;
        try
        {
            source = rand_array[h];
            dest = output_array->data[h];

            TEST_ASSERT(source == dest);
        }
        catch (const Test_failure& ex)
        {
            std::cout << "unmatched set at " << h
                    << " for " << source << " = "
                    << dest << std::endl;
            throw ex;
        }
    }
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::Base64", tests);
}

