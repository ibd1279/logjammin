#pragma once
/*!
 \file lj/Streambuf_mutex.h
 \brief LJ Mutex streambuffer header file.
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

#include <iostream>
#include <mutex>
#include <streambuf>

namespace lj
{
    //! Streambuf base class that supports being treated like a mutex.
    /*!
     \par
     This class provides a streambuf implementation that encompasses a mutex.
     This is used to provided thread safe writing of larger objects to the
     streambuffer. Typical usage would look something like the following code.
     \code{.cpp}
     std::iostream& stream = getStream();
     std::unique_ptr<std::unique_lock<std::mutex> > lock;
     lj::Streambuf_mutex<std::remove_reference<decltype(stream)>::type::char_type>* sbuf =
             dynamic_cast<lj::Streambuf_mutex<std::remove_reference<decltype(stream)>::type::char_type>*>(stream.rdbuf());
     if (sbuf)
     {
        lock.reset(new std::unique_lock<std::mutex>(sbuf->mutex()));
     }
     stream << "some data";
     \endcode
     \note About the example code.
     - a unique_ptr is used to ensure the lock is destroyed/unlocked when the stack unwinds.
     - The decltype is required because iostream may be of a wide or narrow type.
     - The remove references are only required because the iostream was declared as a reference.
     - See documentation on std::unique_lock for additional options on locking
       the mutex.
     \note Deadlocks
     Keep in mind that a thread trying to lock on a mutex it already has will
     cause a deadlock.
     \sa std::ostream& operator<<(std::ostream&, const lj::bson::Node&) for an example.
     \since 0.2
     \date January 4, 2013
     */
    template <typename charT, typename traits=std::char_traits<charT> >
    class Streambuf_mutex : public std::basic_streambuf<charT, traits>
    {
    public:
        typedef charT char_type; //!< helper typedef for iostream compatibility.
        typedef traits traits_type; //!< helper typedef for iostream compatibility.
        typedef typename traits_type::int_type int_type; //!< helper typedef for iostream compatibility.
        typedef typename traits_type::pos_type pos_type; //!< helper typedef for iostream compatibility.
        typedef typename traits_type::off_type off_type; //!< helper typedef for iostream compatibility.
        
        virtual ~Streambuf_mutex()
        {
        }
        
        virtual std::mutex& mutex()
        {
            return mutex_;
        }

    protected:
        Streambuf_mutex() :
                std::basic_streambuf<charT, traits>(),
                mutex_()
        {
        }
    private:
        Streambuf_mutex(const Streambuf_mutex& orig) = delete;
        Streambuf_mutex(Streambuf_mutex&& orig) = delete;
        Streambuf_mutex& operator=(const Streambuf_mutex& orig) = delete;
        Streambuf_mutex& operator=(Streambuf_mutex&& orig) = delete;
        
        std::mutex mutex_;
    }; // class Streambuf_mutex
}; // namespace lj