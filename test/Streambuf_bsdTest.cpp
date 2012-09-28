/*!
 \file test/Streambuf_bsdTest.cpp
 Copyright (c) 2012, Jason Watson
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
#include "lj/Streambuf_bsd.h"

#include <istream>
#include <ostream>
#include <fstream>

#include "test/Streambuf_bsdTest_driver.h"

namespace test
{
    namespace medium
    {
        template<int SZ>
        struct Memory
        {
            Memory() : in_pos_(in_), out_pos_(out_)
            {
            }
            ~Memory()
            {
            }
            int write(const uint8_t* ptr, size_t len)
            {
                size_t len_avail = (out_ + SZ) - out_pos_;
                if (len_avail == 0)
                {
                    return -1;
                }
                len = (out_ + SZ > out_pos_ + len ? len : len_avail);
                memcpy(out_pos_, ptr, len);
                out_pos_ += len;
                return len;
            }
            int read(uint8_t* ptr, size_t len)
            {
                size_t len_avail = (in_ + SZ) - in_pos_;
                if (len_avail == 0)
                {
                    return -1;
                }
                len = (in_ + SZ > in_pos_ + len ? len : len_avail);
                memcpy(ptr, in_pos_, len);
                in_pos_ += len;
                return len;
            }
            std::string error()
            {
                return "Buffer exhausted.";
            }
            char in_[SZ];
            char out_[SZ];
            char* in_pos_;
            char* out_pos_;
        }; // class test::medium::Memory
    }; // namespace test::medium
}; //namespace test

#define MEM_LENGTH (512*1024)

test::medium::Memory<MEM_LENGTH>* random_medium()
{
    std::ifstream rand("/dev/urandom");
    test::medium::Memory<MEM_LENGTH>* mem = new test::medium::Memory<MEM_LENGTH>();
    int len = 0;
    while (len < MEM_LENGTH)
    {
        rand.read((char*)(mem->in_ + len), MEM_LENGTH - len);
        len += rand.gcount();
    }
    return mem;
}

void testRead()
{
    test::medium::Memory<MEM_LENGTH>* mem = random_medium();
    lj::Streambuf_bsd<test::medium::Memory<MEM_LENGTH> > buf(mem, 512, 1);
    std::istream stream(&buf);
    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        int source;
        int dest;
        try
        {
            source = mem->in_[h];
            dest = lj::Streambuf_bsd<test::medium::Memory<MEM_LENGTH> >::traits_type::to_char_type(stream.get());
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
    std::unique_ptr<test::medium::Memory<MEM_LENGTH>> rand(random_medium());

    test::medium::Memory<MEM_LENGTH>* mem = random_medium();
    lj::Streambuf_bsd<test::medium::Memory<MEM_LENGTH> > buf(mem, 1, 512);
    std::ostream stream(&buf);
    stream.write(rand->in_, MEM_LENGTH);
    stream.flush();

    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        int source;
        int dest;
        try
        {
            source = rand->in_[h];
            dest = mem->out_[h];

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
    return Test_util::runner("lj::Streambuf_bsd", tests);
}
