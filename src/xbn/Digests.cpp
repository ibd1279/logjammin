/*!
 \file xbn/Digests.cpp
 \brief LJ Bitcoin Digest Function Implementations.
 \author Jason Watson
 
 Copyright (c) 2013, Jason Watson
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

#include "xbn/Digests.h"
#include "lj/Exception.h"

extern "C" {
#include "nettle/sha2.h"
#include "nettle/ripemd160.h"
}

#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

namespace xbn
{
    void double_sha256(const std::string& input, uint8_t* output)
    {
        // Allocate some stuff on the stack.
        uint8_t buffer[SHA256_DIGEST_SIZE];
        struct sha256_ctx ctx;

        sha256_init(&ctx);
        sha256_update(&ctx, input.size(), reinterpret_cast<const uint8_t*>(input.data()));
        sha256_digest(&ctx, SHA256_DIGEST_SIZE, buffer);
        sha256_update(&ctx, SHA256_DIGEST_SIZE, buffer);
        sha256_digest(&ctx, SHA256_DIGEST_SIZE, output);
    }

    void double_sha256(uint8_t* input, size_t sz, uint8_t* output)
    {
        // Allocate some stuff on the stack.
        uint8_t buffer[SHA256_DIGEST_SIZE];
        struct sha256_ctx ctx;

        sha256_init(&ctx);
        sha256_update(&ctx, sz, input);
        sha256_digest(&ctx, SHA256_DIGEST_SIZE, buffer);
        sha256_update(&ctx, SHA256_DIGEST_SIZE, buffer);
        sha256_digest(&ctx, SHA256_DIGEST_SIZE, output);
    }

    void ripemd160_sha256(const std::string& input, uint8_t* output)
    {
        uint8_t buffer[SHA256_DIGEST_SIZE];
        struct sha256_ctx ctx1;
        struct ripemd160_ctx ctx2;

        sha256_init(&ctx1);
        sha256_update(&ctx1, input.size(), reinterpret_cast<const uint8_t*>(input.data()));
        sha256_digest(&ctx1, SHA256_DIGEST_SIZE, buffer);
        ripemd160_init(&ctx2);
        ripemd160_update(&ctx2, SHA256_DIGEST_SIZE, buffer);
        ripemd160_digest(&ctx2, RIPEMD160_DIGEST_SIZE, output);
    }

    namespace
    {
        inline uint8_t dehex(char c)
        {
            switch (c)
            {
                case '0': return 0;
                case '1': return 1;
                case '2': return 2;
                case '3': return 3;
                case '4': return 4;
                case '5': return 5;
                case '6': return 6;
                case '7': return 7;
                case '8': return 8;
                case '9': return 9;
                case 'a': case 'A': return 10;
                case 'b': case 'B': return 11;
                case 'c': case 'C': return 12;
                case 'd': case 'D': return 13;
                case 'e': case 'E': return 14;
                case 'f': case 'F': return 15;
            }
            std::cout << "char " << c << std::endl;
            throw LJ__Exception(std::string("Invalid hex character."));
        }
    }; // namespace {anonymous}

    std::string as_string(const uint8_t* bytes, const size_t sz)
    {
        std::ostringstream oss;
        for(int h = sz - 1; h >= 0; h--)
        {
            oss << std::hex
                    << std::right
                    << std::setw(2)
                    << std::setfill('0')
                    << (int)bytes[h];
        }
        return oss.str();
    }

    std::unique_ptr<uint8_t[]> as_bytes(const std::string& str, size_t* sz)
    {
        *sz = str.size() >> 1;
        std::unique_ptr<uint8_t[]> result(new uint8_t[*sz]);
        as_bytes(str, result.get(), sz);
        return result;
    }

    void as_bytes(std::string str, uint8_t* result, size_t* sz)
    {
        if (*sz < (str.size() >> 1))
        {
            throw lj::Exception("bcn::as_bytes",
                    "Destination buffer is too small for conversion.");
        }

        *sz = str.size() >> 1;
        uint8_t* end = result + *sz;
        for (uint8_t* ptr = result; ptr < end; ++ptr)
        {
            *ptr = dehex(str.back());
            str.pop_back();
            *ptr |= dehex(str.back()) << 4;
            str.pop_back();
        }
    }
}; // namespace xbn
