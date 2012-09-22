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

extern "C" {
#include "nettle/base64.h"
}

#include <list>
#include <sstream>

namespace lj
{
    uint8_t* base64_decode(const std::string& input, size_t* size)
    {
        // Allocate the necessary memory.
        unsigned max_size = BASE64_DECODE_LENGTH(input.size());
        uint8_t* output = new uint8_t[max_size];
        
        // Base64 decode the data.
        struct base64_decode_ctx ctx;
        base64_decode_init(&ctx);
        
        // base64_decode_update requires that the dst_size value be initialized
        // to the size of output.
        int success = base64_decode_update(&ctx,
                &max_size,
                output,
                input.size(),
                reinterpret_cast<const uint8_t*>(input.c_str()));
        if (!success && !base64_decode_final(&ctx)) {
            throw LJ__Exception("Unable to decode input string.");
        }
        
        // return the result.
        *size = max_size;
        return output;
    }

    std::string base64_encode(const uint8_t* input, size_t size)
    {
        // Allocate the necessary memory.
        unsigned max_size = BASE64_ENCODE_LENGTH(size) + BASE64_ENCODE_FINAL_LENGTH;
        uint8_t* output = new uint8_t[max_size];
       
        // Base64 encode the data.
        struct base64_encode_ctx ctx;
        base64_encode_init(&ctx);
        unsigned update_size = base64_encode_update(&ctx,
                output,
                size,
                input);
        unsigned final_size = base64_encode_final(&ctx,
                output + update_size);
        
        // clean up the memory and return the result.
        std::string tmp(reinterpret_cast<char*>(output), update_size + final_size);
        delete[] output;
        return tmp;
    }
}; // namespace lj
