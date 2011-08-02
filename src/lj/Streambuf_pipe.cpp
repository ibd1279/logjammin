/*!
 \file lj/Streambuf_pipe.cpp
 \brief LJ pipe stream buffer implementation.
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
#include "lj/Streambuf_pipe.h"

namespace lj
{
    Streambuf_pipe::Streambuf_pipe() : std::streambuf(),
            i_(std::ios_base::out|std::ios_base::in|std::ios_base::binary),
            o_(std::ios_base::out|std::ios_base::in|std::ios_base::binary)
    {
        // All set to null to force under/over-flow states.
        this->setp(NULL, NULL);
        this->setg(NULL, NULL, NULL);
    }

    Streambuf_pipe::~Streambuf_pipe()
    {
    }

    int Streambuf_pipe::underflow()
    {
        int c = i_.get();
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            return c;
        }
        return traits_type::eof();
    }

    int Streambuf_pipe::overflow(int c)
    {
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            o_.put(traits_type::to_char_type(c));
        }
        return c;
    }

    std::ostream& Streambuf_pipe::sink()
    {
        return i_;
    }

    std::istream& Streambuf_pipe::source()
    {
        return o_;
    }
}; // namespace lj
