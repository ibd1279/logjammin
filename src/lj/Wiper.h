#pragma once
/*!
 \file lj/Wiper.h
 \brief LJ class for automatically freeing and wiping arrays.
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

#include <memory>

namespace lj
{

    template<typename T, typename D = std::default_delete<T> >
            class Wiper
    {
    public:
        Wiper() = default;

        void operator()(T* t) const
        {
            wipe(t);
            D deleter;
            deleter(t);
        }

        static void wipe(T* t)
        {
            size_t sz = sizeof (T);
            volatile uint8_t* p = ((uint8_t*) t) + sz;
            while (sz--)
            {
                *(--p) = 0;
            }
        }

        static inline void wipe(std::unique_ptr<T>& t)
        {
            wipe(t.get());
        }
    };

    template<typename T, typename D>
    class Wiper<T[], D>
    {
    public:

        Wiper() : count_(1)
        {
        }

        void operator()(T* t) const
        {
            wipe(t, count_);
            D deleter;
            deleter(t);
        }

        static void wipe(T* t, const size_t count)
        {
            size_t sz = sizeof (T) * count;
            volatile uint8_t* p = ((uint8_t*) t) + sz;
            while (sz--)
            {
                *(--p) = 0;
            }
        }

        static inline void wipe(std::unique_ptr<T[]>& t, const size_t count)
        {
            wipe(t.get(), count);
        }

        void set_count(const size_t count)
        {
            count_ = count;
        }

        inline size_t count() const
        {
            return count_;
        }
    private:
        size_t count_;
    };
}; // namespace lj