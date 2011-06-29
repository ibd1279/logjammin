#pragma once
/*!
 \file lj/Bio_streambuf.h
 \brief LJ BIO streambuf header and implementation.
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

#include "lj/Exception.h"
#include "lj/Log.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <streambuf>

extern "C"
{
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
}

namespace lj
{
    //! Get the current openssl error strings.
    /*!
     \par
     Convert openssl error messages from internal buffers to a 
     stl string.
     \return The OpenSSL error buffer as a string.
     */
    std::string openssl_get_error_string()
    {
        BIO* mem = BIO_new(BIO_s_mem());
        ERR_print_errors(mem);
        BUF_MEM* buf;
        BIO_get_mem_ptr(mem, &buf);
        return std::string(buf->data);
    }

    //! streambuf implementation for BIO sockets.
    /*!
     \par
     Allows for stl stream manipulation of BIO sockets.
     \tparam charT The type of character to use, wide or narrow
     \tparam traits The namespace used for referencing the traits of the stream.
     \author Jason Watson
     \version 1.0
     \date June 28, 2011
     */
    template < typename charT=char, typename traits=std::char_traits<charT> >
    class Bio_streambuf : public std::basic_streambuf<charT, traits>
    {
    public:

        typedef traits traits_type;
        typedef typename traits_type::int_type int_type;
        typedef typename traits_type::pos_type pos_type;
        typedef typename traits_type::off_type off_type;

        //! Create a new BIO streambuf
        /*!
         \par
         The \c io pointer is expected to be created and fully functional.
         \par
         The values of \c in_sz and \c out_sz are used to control the size
         of the buffers used for input and output operations.
         \param io The BIO object.
         \param sz The maximum size of the buffer.
         */
        Bio_streambuf(BIO* io, const size_t in_sz, const size_t out_sz)
                : io_(io), in_size_(in_sz), out_size_(out_sz)
        {
            assert(sizeof(charT) == 1);
            in_ = new charT[in_size_];
            out_ = new charT[out_size_ + 1];

            // force and underflow on first read.
            this->setg(0, 0, 0);

            // setup writes properly.
            this->setp(out_, out_ + out_size_);
        }

        //! deleted copy constructor
        Bio_streambuf(const Bio_streambuf&) = delete;

        //! deleted assignment operator
        Bio_streambuf& operator=(const Bio_streambuf&) = delete;
        
        //! Destructor
        virtual ~Bio_streambuf()
        {
            if (in_)
            {
                delete[] in_;
            }
            if (out_)
            {
                delete[] out_;
            }
            BIO_free(io_);
        }
    protected:
        virtual int_type overflow(int_type c = traits::eof())
        {
            const charT* start = out_;
            charT* end = this->pptr();

            // deal with the character provided on the parameter.
            if (!traits_type::eq_int_type(c, traits_type::eof()))
            {
                *end++ = traits_type::to_char_type(c);
            }

            // write some number of bytes to the underlying BIO.
            const int len = end - start;
            assert(len >= 0);

            const int sent_bytes = BIO_write(io_, start, len);
            const int unsent_bytes = len - sent_bytes;
            if (0 >= sent_bytes)
            {
                if (!BIO_should_retry(io_))
                {
                    Log::debug("Unrecoverable BIO write error: %s") <<
                            openssl_get_error_string() <<
                            Log::end;
                    return traits::eof();
                }
                else
                {
                    Log::debug("Recoverable(?) BIO write error: %s") <<
                            openssl_get_error_string() <<
                            Log::end;
                    return traits::eof();
                }
            }

            // deal with moving the unsent bytes to the beginning of the
            // buffer. 
            if (len > sent_bytes)
            {
                memmove(out_, out_ + sent_bytes, unsent_bytes);
            }

            // Set the pointers to the un-used part of the buffer.
            this->setp(out_ + unsent_bytes, out_ + out_size_);
            return traits_type::not_eof(c);
        }
        virtual int_type sync()
        {
            const charT* start = out_;
            const charT* end = this->pptr();

            const int len = end - start;
            assert(len >= 0);

            int sent_bytes = 0;
            while (sent_bytes < len)
            {
                const int nbytes = BIO_write(io_, start + sent_bytes,
                        len - sent_bytes);
                if (0 >= nbytes)
                {
                    Log::debug("Unrecoverable BIO sync error: %s") <<
                            openssl_get_error_string() <<
                            Log::end;
                    return traits::eof();
                }
                sent_bytes += nbytes;
            }

            // reset the pointers for a blank buffer.
            this->setp(out_, out_ + out_size_);
            return 0;
        }
        virtual int_type underflow()
        {
            const charT* start = this->gptr();
            const charT* end = this->egptr();

            const int len = end - start;
            assert(len >= 0);

            // Move any unread bytes to the beginning of the buffer.
            if (0 < len)
            {
                memmove(in_, start, len);
            }

            // Read as many bytes as possible from the BIO.
            const int recv_bytes = BIO_read(io_, in_ + len, in_size_ - len);
            if (0 >= recv_bytes)
            {
                Log::debug("Unrecoverable BIO read error: %s") <<
                        openssl_get_error_string() <<
                        Log::end;
                return traits::eof();
            }

            this->setg(in_, in_, in_ + len + recv_bytes);
            return traits_type::not_eof(*in_);
        }
    private:
        BIO* io_;
        const size_t in_size_;
        const size_t out_size_;
        charT* in_;
        charT* out_;
    };
}; // namespace lj
