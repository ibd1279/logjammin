#pragma once
/*!
 \file lj/Streambuf_bsd.h
 \brief LJ BSD Socket streambuffer.
 \author Jason Watson

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

#include "lj/Exception.h"
#include "lj/Log.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <streambuf>

extern "C"
{
#include <unistd.h>
#include <sys/socket.h>
}

namespace lj
{
    namespace medium
    {
        class Socket
        {
        public:
            Socket(int data) : fd(data)
            {
            }
            ~Socket()
            {
                ::close(fd);
            }
            int write(const uint8_t* ptr, size_t len)
            {
                return ::send(fd, ptr, len, 0);
            }
            int read(uint8_t* ptr, size_t len)
            {
                return ::recv(fd, ptr, len, 0);
            }
            std::string error()
            {
                return ::strerror(errno);
            }
        private:
            int fd;
        };
    }; // namespace lj::medium

    //! streambuf implementation for BSD sockets.
    /*!
     \par
     Allows for stl stream manipulation of BSD sockets.
     \par
     This can technically be used for buffering any type of input or output.
     The provided \c mediumT class is used to perform the actual read and write
     operations. \c lj::medium::socket provides an example medium type for use
     with this streambuf.
     \tparam charT The type of character to use, wide or narrow
     \tparam traits The namespace used for referencing the traits of the stream.
     \tparam mediumT The medium to use for reading and writing.
     \author Jason Watson
     \since 0.2
     \date September 27, 2012
     */
    template <typename mediumT, typename charT=char, typename traits=std::char_traits<charT> >
    class Streambuf_bsd : public std::basic_streambuf<charT, traits>
    {
    public:
        typedef traits traits_type;
        typedef typename traits_type::int_type int_type;
        typedef typename traits_type::pos_type pos_type;
        typedef typename traits_type::off_type off_type;

        //! Create a new streambuf object around a BSD socket.
        /*!
         \par
         Buffer size is measured in terms of the character type associated with
         the stream buffer (e.g. char, wchar, etc.). It is not the number of
         bytes associated with the buffer.
         \par
         The \c Streambuf_bsd object assumes responsibility for releasing the
         \c medium object.
         \param medium Pointer to the underlying medium.
         \param in_sz The size of the read buffer.
         \param out_sz The size of the writer buffer.
         */
        Streambuf_bsd(mediumT* medium, const size_t in_sz, const size_t out_sz) :
                medium_(medium), in_size_(in_sz), out_size_(out_sz)
        {
            assert(sizeof(charT) == 1);

            in_ = new charT[in_size_];
            out_ = new charT[out_size_ + 1];

            // force and underflow on first read.
            this->setg(0, 0, 0);

            // setup writes properly.
            this->setp(out_, out_ + out_size_);

            in_buffer_[0] = 0;
            out_buffer_[0] = 0;
        }

        //! Copy constructor explicitly deleted.
        /*!
         \param o The other object.
         */
        Streambuf_bsd(const Streambuf_bsd& o) = delete;

        //! Move constructor
        /*!
         \param o The other object.
         */
        Streambuf_bsd(Streambuf_bsd&& o) :
                std::basic_streambuf<charT, traits>(o),
                medium_(std::move(o.medium_)),
                in_size_(o.in_size_),
                out_size_(o.out_size_),
                in_(o.in_),
                out_(o.out_)
        {
            o.in_ = nullptr;
            o.out_ = nullptr;
            std::memcpy(in_buffer_, o.in_buffer_, sizeof(charT));
            std::memcpy(out_buffer_, o.out_buffer_, sizeof(charT));
        }

        //! Clean up our resources.
        virtual ~Streambuf_bsd()
        {
            if (in_)
            {
                delete[] in_;
            }
            if (out_)
            {
                delete[] out_;
            }
        }

        //! Copy assignment operator explicitly deleted.
        /*!
         \param o The other object.
         */
        Streambuf_bsd& operator=(const Streambuf_bsd& o) = delete;

        //! Move assignment operator
        /*!
         \param o The other object.
         */
        Streambuf_bsd& operator=(Streambuf_bsd&& o)
        {
            std::basic_streambuf<charT, traits>::operator=(o);
            medium_ = std::move(o.medium_);
            in_size_ = o.in_size_;
            out_size_ = o.out_size_;
            in_ = o.in_;
            out_ = o.out_;
            std::memcpy(in_buffer_, o.in_buffer_, sizeof(charT));
            std::memcpy(out_buffer_, o.out_buffer_, sizeof(charT));

            o.in_ = nullptr;
            o.out_ = nullptr;
        }
    private:
        void send_buffer(bool retry)
        {
            // Loop on the character buffer. it should be small,
            // and some part of the character has already been sent.
            while (out_buffer_[0] > 0 && retry)
            {
                // try to send the remaining bytes of the out buffer.
                const int bytes_sent = medium_->write(
                        out_buffer_ + 1,
                        out_buffer_[0]);

                if (0 > bytes_sent)
                {
                    // If we got something less than 0, we have an error.
                    std::ostringstream oss;
                    oss << "Unable to send BSD character buffer. [";
                    oss << medium_->error() << "]";
                    throw LJ__Exception(oss.str());
                }
                else if (0 < bytes_sent)
                {
                    // If we sent something greater than 0, we need to compact
                    // the character buffer.
                    std::memmove(out_buffer_ + 1,
                            out_buffer_ + 1 + bytes_sent,
                            out_buffer_[0] - bytes_sent);
                    out_buffer_[0] -= bytes_sent;
                }
                //if 0 bytes were sent, loop again and retry.
            }
        }
    protected:
        virtual int_type overflow(int_type c = traits::eof()) override
        {
            // if there are any buffered bytes, send those first.
            try
            {
                send_buffer(true);
            }
            catch (const lj::Exception& ex)
            {
                log::out<Info>(ex.str());
                return traits::eof();
            }

            // Get the useful buffer pointers.
            const charT* start = out_;
            charT* end = this->pptr();

            // deal with the character provided on the parameter.
            if (!traits_type::eq_int_type(c, traits_type::eof()))
            {
                *end++ = traits_type::to_char_type(c);
            }

            // since the transport uses bytes, not chars, we convert the pointers.
            uint8_t* byte_start = (uint8_t*)start;
            const uint8_t* byte_end = (uint8_t*)end;
            const ptrdiff_t len = byte_end - byte_start;
            assert(len >= 0); // ptrdiff_t is normally signed, but we don't want negative values.

            // push some number of bytes onto the socket.
            int sent_bytes;
            do
            {
                sent_bytes = medium_->write(byte_start, (size_t)len);
            }
            while(sent_bytes == 0);

            if (0 > sent_bytes)
            {
                // deal with the error cases.
                log::format<Info>("Unrecoverable BSD write error: [%s]")
                        << medium_->error()
                        << log::end;
                return traits::eof();
            }

            // Process the bytes of the sheared character, if any.
            const uint8_t sheared_bytes = sent_bytes % sizeof(charT);
            if (0 < sheared_bytes)
            {
                // Copy any sheared bytes into the out buffer.
                std::memcpy(out_buffer_ + 1, byte_start + sent_bytes, sheared_bytes);
                out_buffer_[0] = sheared_bytes;

                // Sheared bytes are considered sent bytes because the character
                // they compose has been removed form the underlying memory.
                sent_bytes += sheared_bytes;
            }

            // compact the underlying memory to make room for more data. This
            // is only necessary if some bytes remained unsent.
            const size_t unsent_bytes = len - sent_bytes;
            if (0 < unsent_bytes)
            {
                std::memmove(byte_start, byte_start + sent_bytes, unsent_bytes);
            }

            // Set the pointers to the un-used part of the buffer.
            const int unsent_chars = unsent_bytes / sizeof(charT);
            this->setp(out_ + unsent_chars, out_ + out_size_);

            // Make one last effort to get the sheared bytes out before we return.
            try
            {
                send_buffer(false);
            }
            catch (const lj::Exception& ex)
            {
                log::out<Info>(ex.str());
                return traits::eof();
            }

            return traits_type::not_eof(c);
        }
        virtual int_type sync() override
        {
            // if there are any buffered bytes, send those first.
            try
            {
                send_buffer(true);
            }
            catch (const lj::Exception& ex)
            {
                log::out<Info>(ex.str());
                return traits::eof();
            }

            // try to send buffered characters next.
            const charT* start = out_;
            const charT* end = this->pptr();

            // since the transport uses bytes, not chars, we convert the pointers.
            uint8_t* byte_start = (uint8_t*)start;
            const uint8_t* byte_end = (uint8_t*)end;

            // loop over send until we have sent everything on the transport.
            do
            {
                const ptrdiff_t len = byte_end - byte_start;
                assert(len >= 0); // ptrdiff_t is normally signed, but we don't want negative values.
                const int sent_bytes = medium_->write(byte_start, (size_t)len);
                if (0 > sent_bytes)
                {
                    log::format<Info>("Unrecoverable BSD sync error: [%s]")
                            << medium_->error()
                            << log::end;
                    return traits::eof();
                }
                byte_start += sent_bytes;
            }
            while(byte_start < byte_end);

            // reset the pointers for a blank buffer.
            this->setp(out_, out_ + out_size_);
            return 0;
        }
        virtual int_type underflow() override
        {
            // how many bytes are currently unread in the buffer.
            const uint8_t* unread_start = (uint8_t*)this->gptr();
            const uint8_t* unread_end = (uint8_t*)this->egptr();
            ptrdiff_t unread_bytes = unread_end - unread_start;
            assert(unread_bytes >= 0); // ptrdiff_t is normally signed, but we don't want negative values.

            // Move any unread bytes to the beginning of the buffer.
            if (0 < unread_bytes)
            {
                std::memmove(in_, unread_start, unread_bytes);
            }

            // copy any sheared bytes to the buffer.
            if (0 < in_buffer_[0])
            {
                std::memcpy(in_ + unread_bytes, in_buffer_ + 1, in_buffer_[0]);
                unread_bytes += in_buffer_[0];
            }

            // figure out where we can start storing some data.
            uint8_t* start = ((uint8_t*)in_) + unread_bytes;
            const uint8_t* end = ((uint8_t*)(in_ + in_size_));
            const ptrdiff_t len = end - start;
            assert(len >= 0); // ptrdiff_t is normally signed, but we don't want negative values.

            // Read as many bytes as possible from the BIO.
            int recv_bytes;
            do
            {
                recv_bytes = medium_->read(start, len);
            }
            while(recv_bytes == 0);

            if (0 > recv_bytes)
            {
                log::format<Debug>("Unrecoverable BSD read error: [%s]")
                        << medium_->error()
                        << log::end;
                return traits::eof();
            }

            start = (uint8_t*)in_;
            end = start + recv_bytes;
            ptrdiff_t available_bytes = end - start;

            const uint8_t sheared_bytes = available_bytes % sizeof(charT);
            if (0 < sheared_bytes)
            {
                // Copy any sheared bytes into the buffer.
                std::memcpy(in_buffer_ + 1, end - sheared_bytes, sheared_bytes);
                in_buffer_[0] = sheared_bytes;

                // Sheared bytes are considered unread bytes because the character
                // they compose has not been fully read form the underlying memory.
                available_bytes -= sheared_bytes;
                end -= sheared_bytes;
            }

            // reset the reading buffer.
            this->setg(in_, in_, (charT*)end);
            return traits_type::not_eof(*in_);
        }

    private:

        std::unique_ptr<mediumT> medium_;
        const size_t in_size_;
        const size_t out_size_;
        charT* in_;
        charT* out_;
        uint8_t in_buffer_[sizeof(charT)]; //!< buffer for any left over bytes. first byte is the number of bytes buffered.
        uint8_t out_buffer_[sizeof(charT)]; //!< buffer for any left over bytes. first byte is the number of bytes buffered.
    }; // class lj::Streambuf_bsd
}; // namespace lj