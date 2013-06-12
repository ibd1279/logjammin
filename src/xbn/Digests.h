#pragma once
/*!
 \file xbn/Digests.h
 \brief LJ Bitcoin Digest Functions.
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

#include <cstdint>
#include <list>
#include <string>

namespace xbn
{
    //! Bitcoin Longer hash.
    /*!
     \code
     sha256(sha256(input));
     \endcode
     \param input String to hash.
     \param [out] output The resulting hash. Must have space for 32 bytes.
     \throws lj::Exception on failures.
     */
    void double_sha256(const std::string& input, uint8_t* output);

    //! Bitcoin Shorter hash.
    /*!
     \code
     ripemd160(sha256(input));
     \endcode
     \param input String to hash.
     \param [out] output The resulting hash. Must have space for 20 bytes.
     \throws lj::Exception on failures.
     */
    void ripemd160_sha256(const std::string& input, uint8_t* output);

    //! Convert a byte array to a base 16 string.
    /*!
     This method will change the order of the bytes on the output
     to little-endian. On x86, this means they are reversed.
     \param bytes The bytes to convert.
     \param sz The number of bytes to convert.
     \return The converted string.
     */
    std::string as_string(const uint8_t* bytes, const size_t sz);

    //! Convert a base16 string to bytes.
    /*!
     This method will change the order of the bytes from 
     little-endian to the host order. On x86, this means they are
     reversed.
     \param str The string to convert. Must be base16.
     \param[out] sz The size of the resulting bytes;
     \return A unique_ptr to the bytes.
     \throws lj::Exception If \c str contains non base16 characters.
     */
    std::unique_ptr<uint8_t[]> as_bytes(const std::string& str, size_t* sz);

    //! Convert a base16 string to bytes.
    /*!
     This method will change the order of the bytes from little-endian
     to the host order. On x86, this means they are reversed.
     \param str The input string.
     \param[out] result Where you want the result values stored.
     \param[in,out] sz Must be set to the size of \c result going in.
     Will be set to the size used upon return.
     */
    void as_bytes(std::string str, uint8_t* result, size_t* sz);
}; // namespace xbn
