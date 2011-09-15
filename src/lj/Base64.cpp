/*!
 \file lj/Base64.cpp
 \brief LJ base 64 implementation.
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

#include "lj/Base64.h"
#include "lj/Exception.h"
#include "cryptopp/base64.h"
#include <list>
#include <sstream>

namespace lj
{
    uint8_t* base64_decode(const std::string& input, size_t* size)
    {
        std::string buffer;
        CryptoPP::StringSource(reinterpret_cast<const uint8_t*>(input.data()), input.size(), true,
                new CryptoPP::Base64Decoder(
                        new CryptoPP::StringSink(buffer)));
        *size = buffer.size();
        uint8_t* data = new uint8_t[*size];
        memcpy(data, buffer.data(), *size);
        return data;
    }

    std::string base64_encode(const uint8_t* input, size_t size)
    {
        std::string buffer;
        CryptoPP::StringSource(input, size, true,
                new CryptoPP::Base64Encoder(
                        new CryptoPP::StringSink(buffer),
                        false));
        return buffer;
    }
}; // namespace lj
