#pragma once
/*!
 \file lj/Streambuf_pipe.h
 \brief LJ pipe stream buffer header.
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
#include <istream>
#include <ostream>
#include <sstream>

namespace lj
{
    //! Pipe streambuf for use with iostreams.
    /*!
     \author Jason Watson
     \version 1.0
     \date August 1, 2011
     */
    class Streambuf_pipe : public std::streambuf
    {
    public:
        //! Create a new pipe streambuf.
        Streambuf_pipe();

        //! Destructor.
        virtual ~Streambuf_pipe();

        //! Data sink.
        /*!
         /par
         Allows writing values into the pipe.
         \return An output stream.
         */
        virtual std::ostream& sink();
        //! Data source.
        /*!
         \par
         Allows reading values from the pipe.
         \return An input stream.
         */
        virtual std::istream& source();
    protected:
        //! std::streambuf override.
        /*!
         \return character read or EOF.
         */
        virtual int underflow();

        //! std::streambuf override.
        /*!
         \return character written or EOF.
         */
        virtual int overflow(int c = EOF);
    private:
        std::stringstream i_;
        std::stringstream o_;
        char ibuf_[1];
    };
}; // namespace lj
