/*!
 \file lj/Uuid.cpp
 \brief LJ Uuid implementation.
 \author Jason Watson
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

#include "Uuid.h"

#include <cstdlib>
#include <ios>
#include <iostream>
#include <list>
#include <fstream>
#include <sstream>

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/err.h>
}

namespace lj
{
    const Uuid Uuid::k_nil{0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0};
    const Uuid Uuid::k_ns_dns{0x6B,0xA7,0xB8,0x10, 0x9D,0xAD,0x11,0xD1, 0x80,0xB4,0x00,0xC0, 0x4F,0xD4,0x30,0xC8};
    const Uuid Uuid::k_ns_url{0x6B,0xA7,0xB8,0x11, 0x9D,0xAD,0x11,0xD1, 0x80,0xB4,0x00,0xC0, 0x4F,0xD4,0x30,0xC8};
    const Uuid Uuid::k_ns_oid{0x6B,0xA7,0xB8,0x12, 0x9D,0xAD,0x11,0xD1, 0x80,0xB4,0x00,0xC0, 0x4F,0xD4,0x30,0xC8};
    const Uuid Uuid::k_ns_x500{0x6B,0xA7,0xB8,0x14, 0x9D,0xAD,0x11,0xD1, 0x80,0xB4,0x00,0xC0, 0x4F,0xD4,0x30,0xC8};
    
    Uuid::Uuid()
    {
        std::ifstream rand("/dev/urandom");
        for (int i = 0; i < 16; ++i)
        {
            data_[i] = static_cast<uint8_t>(rand.get() & 0xffULL);
        }

        // setting the version.
        data_[6] &= 0x0f;
        data_[6] |= 0x40;

        // setting the y value.
        data_[8] &= 0xbf;
        data_[8] |= 0x80;
    }

    Uuid::Uuid(std::initializer_list<uint8_t> d)
    {
        int i = 0;
        for (auto iter = d.begin();
                d.end() != iter && i < 16;
                ++iter, ++i)
        {
            data_[i] = *iter;
        }
        for (; i < 16; ++i)
        {
            data_[i] = 0;
        }
    }

    Uuid::Uuid(const uint8_t d[16])
    {
        for (int i = 0; i < 16; ++i)
        {
            data_[i] = d[i];
        }
    }

    Uuid::Uuid(const Uuid& o)
    {
        for (uint8_t i = 0; i < 16; ++i)
        {
            data_[i] = o.data_[i];
        }
    }

    Uuid::Uuid(const std::string& o)
    {
        std::list<std::string> byte_pairs;
        bool is_first = true;
        for (auto iter = o.begin();
             o.end() != iter;
             ++iter)
        {
            if (*iter == '-' || *iter == '{')
            {
                continue;
            }
            else if (*iter == '}')
            {
                break;
            }

            if (!is_first)
            {
                std::string& byte = byte_pairs.back();
                byte.push_back(*iter);
                is_first = true;
            }
            else
            {
                std::string byte;
                byte.push_back(*iter);
                byte_pairs.push_back(byte);
                is_first = false;
            }
        }

        int i = 0;
        for (auto iter = byte_pairs.begin();
             byte_pairs.end() != iter && i < 16;
             ++iter, ++i)
        {
            data_[i] = static_cast<uint8_t>(strtoul((*iter).c_str(),
                                                    NULL,
                                                    16));
        }
        for (; i < 16; ++i)
        {
            data_[i] = 0;
        }
    }
    
    Uuid::Uuid(const Uuid& ns, const void* name, const size_t name_sz)
    {
        size_t ns_sz;
        const uint8_t* ns_ptr = ns.data(&ns_sz);
        
        SHA_CTX ctx;
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, ns_ptr, ns_sz);
        SHA1_Update(&ctx, name, name_sz);
        
        uint8_t tmp[20];
        SHA1_Final(tmp, &ctx);
        memcpy(data_, tmp, 16);
        
        data_[6] &= 0x0f; // time high/ver.
        data_[6] |= 0x50;
        data_[8] &= 0x3f; //clock seq high/reserved
        data_[8] |= 0x80;
    }

    Uuid::Uuid(const uint64_t o)
    {
        // Load the provided key into the data array, avoiding
        // the "reserved" bits in the UUID format.
        data_[0] = static_cast<uint8_t>((o & 0xff00000000000000ULL) >> 56ULL);
        data_[1] = static_cast<uint8_t>((o & 0x00ff000000000000ULL) >> 48ULL);
        data_[2] = static_cast<uint8_t>((o & 0x0000ff0000000000ULL) >> 40ULL);
        data_[3] = static_cast<uint8_t>((o & 0x000000ff00000000ULL) >> 32ULL);
        data_[4] = static_cast<uint8_t>((o & 0x00000000ff000000ULL) >> 24ULL);
        data_[5] = static_cast<uint8_t>((o & 0x0000000000ff0000ULL) >> 16ULL);
        data_[6] = static_cast<uint8_t>((o & 0x000000000000f000ULL) >> 12ULL);
        data_[7] = static_cast<uint8_t>((o & 0x0000000000000ff0ULL) >> 4ULL);
        data_[8] = static_cast<uint8_t>((o & 0x000000000000000fULL) << 2ULL);

        // Populate everything else with random values.
        std::ifstream rand("/dev/urandom");
        data_[8] |= static_cast<uint8_t>(rand.get() & 0x03ULL);

        for (int i = 9; i < 16; ++i)
        {
            data_[i] = static_cast<uint8_t>(rand.get() & 0xffULL);
        }

        // setting the version.
        data_[6] &= 0x0f;
        data_[6] |= 0x40;

        // setting the y value.
        data_[8] &= 0xbf;
        data_[8] |= 0x80;
    }

    Uuid::~Uuid()
    {
    }

    Uuid& Uuid::operator=(const Uuid& o)
    {
        memcpy(data_, o.data_, 16);
        return *this;
    }

    bool Uuid::operator==(const Uuid& o) const
    {
        for (uint8_t i = 0; i < 16; ++i)
        {
            if (data_[i] != o.data_[i])
            {
                return false;
            }
        }
        return true;
    }

    bool Uuid::operator<(const Uuid& o) const
    {
        for (uint8_t i = 0; i < 16; ++i)
        {
            if (data_[i] < o.data_[i])
            {
                return true;
            }
        }
        return false;
    }

    Uuid::operator std::string() const
    {
        char buf[40];
        sprintf(buf, "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                data_[0], data_[1], data_[2], data_[3],
                data_[4], data_[5], data_[6], data_[7],
                data_[8], data_[9], data_[10],data_[11],
                data_[12], data_[13], data_[14], data_[15]);
        return std::string(buf);
    }

    Uuid::operator uint64_t() const
    {
        uint64_t key = 0;
        key |= static_cast<uint64_t>(data_[0]) << 56ULL;
        key |= static_cast<uint64_t>(data_[1]) << 48ULL;
        key |= static_cast<uint64_t>(data_[2]) << 40ULL;
        key |= static_cast<uint64_t>(data_[3]) << 32ULL;
        key |= static_cast<uint64_t>(data_[4]) << 24ULL;
        key |= static_cast<uint64_t>(data_[5]) << 16ULL;
        key |= static_cast<uint64_t>(data_[6] & 0x0f) << 12ULL;
        key |= static_cast<uint64_t>(data_[7]) << 4ULL;
        key |= static_cast<uint64_t>(data_[8] & 0x3c) >> 2ULL;
        return key;
    }

    std::string Uuid::str() const
    {
        uint64_t key = 0;
        key |= static_cast<uint64_t>(data_[0]) << 56ULL;
        key |= static_cast<uint64_t>(data_[1]) << 48ULL;
        key |= static_cast<uint64_t>(data_[2]) << 40ULL;
        key |= static_cast<uint64_t>(data_[3]) << 32ULL;
        key |= static_cast<uint64_t>(data_[4]) << 24ULL;
        key |= static_cast<uint64_t>(data_[5]) << 16ULL;
        key |= static_cast<uint64_t>(data_[6] & 0x0f) << 12ULL;
        key |= static_cast<uint64_t>(data_[7]) << 4ULL;
        key |= static_cast<uint64_t>(data_[8] & 0x3c) >> 2ULL;

        std::ostringstream oss;
        oss << ((std::string)(*this)) << "/" << key;
        return oss.str();
    }
};
