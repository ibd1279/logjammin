/*!
 \file Base64.cpp
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

#include <list>
#include <sstream>

namespace lj
{
    namespace 
    {
        static const char base64_values[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        static const char base64_dictionary[] = {
        /* + */ 62,-1,-1,-1,
        /* / */ 63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,-1,
        /* A */  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,
        /* a */ 26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1};
    }; // namespace
    
    // Simple uninteligent implementation.
    unsigned char *base64_decode(const std::string &input, unsigned int *size)
    {
        unsigned int i = 0;
        std::list<unsigned char> buffer;
        std::string::const_iterator iter = input.begin();
        while (input.end() != iter && '=' != *iter)
        {
            char tmp[4];
            i = 0;
            for (;
                 input.end() != iter && '=' != *iter && 4 > i;
                 ++i, ++iter)
            {
                if ('+' > *iter)
                {
                    throw new lj::Exception("base64", "invalid character");
                }
                else if (base64_dictionary[*iter - '+'] == -1)
                {
                    throw new lj::Exception("base64", "invalid character");
                }
                tmp[i] = base64_dictionary[*iter - '+'];
            }
            
            switch (i)
            {
                case 4:
                    buffer.push_back((unsigned char)((tmp[0] << 2) | (tmp[1] >> 4)));
                    buffer.push_back((unsigned char)((tmp[1] << 4) | (tmp[2] >> 2)));
                    buffer.push_back((unsigned char)((tmp[2] << 6) | tmp[3]));
                    break;
                case 3:
                    buffer.push_back((unsigned char)((tmp[0] << 2) | (tmp[1] >> 4)));
                    buffer.push_back((unsigned char)((tmp[1] << 4) | (tmp[2] >> 2)));
                    break;
                case 2:
                    buffer.push_back((unsigned char)((tmp[0] << 2) | (tmp[1] >> 4)));
                    break;
                case 1:
                    throw new lj::Exception("base64", "invalid end character");
            }
        }
        
        *size = buffer.size();
        unsigned char* result = new unsigned char[*size + 1];
        result[*size] = 0;
        
        i = 0;
        for (std::list<unsigned char>::const_iterator iter = buffer.begin();
             iter != buffer.end();
             ++iter, ++i)
        {
            result[i] = *iter;
        }
        
        return result;
    }

    std::string base64_encode(const unsigned char *input, unsigned int size)
    {        
        std::ostringstream data;
        unsigned int h = 0;
        if (2 < size)
        {
            while (h < size - 2)
            {
                data << base64_values[input[h] >> 2];
                data << base64_values[((input[h] & 3) << 4)  | (input[h + 1] >> 4)];
                data << base64_values[((input[h + 1] & 15) << 2) | (input[h + 2] >> 6)];
                data << base64_values[input[h + 2] & 63];
                h += 3;
            }
        }
        
        switch (size - h)
        {
            case 1:
                data << base64_values[input[h] >> 2];
                data << base64_values[(input[h] & 3) << 4];
                data << "==";
                break;
            case 2:
                data << base64_values[input[h] >> 2];
                data << base64_values[((input[h] & 3) << 4)  | (input[h + 1] >> 4)];
                data << base64_values[(input[h + 1] & 15) << 2];
                data << "=";
                break;
        }
        
        return data.str();
    }
}; // namespace lj