#pragma once
/*!
 \file lj/Wiper.h
 \brief LJ class for automatically freeing and wiping arrays.
 \author Jason Watson

 Copyright (c) 2014, Jason Watson
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

    /*!
     \brief Memory wiping implementation.

     writes zeros to the provided memory area before releasing the memory.

     \tparam T the type to be wiped.
     \tparam D the "delete" provider type.
     */
    template<typename T, typename D = std::default_delete<T> >
    class Wiper
    {
    public:
        //! Default constructor.
        Wiper() = default;

        /*!
         \brief Functor operator.

         This primarily allows this class to act as a replacement for the default
         deleter on std::unique_ptr objects.
         This invokes the wipe and the delete.

         \param t The object to wipe and delete.
         */
        inline void operator()(T* t) const
        {
            wipe(t);
            D deleter;
            deleter(t);
        }

        /*!
         \brief wipe method.

         write zeros to a range of memory.

         \param t Typed pointer to the memory to wipe.
         \*/
        static void wipe(T* t)
        {
            size_t sz = sizeof (T);
            volatile uint8_t* p = ((uint8_t*) t) + sz;
            while (sz--)
            {
                *(--p) = 0;
            }
        }

        /*!
         \brief wipe method.

         write zeros to a range of memory.

         \param t The std::unique_ptr to an object to wipe.
         \*/
        static inline void wipe(std::unique_ptr<T>& t)
        {
            wipe(t.get());
        }
    }; // class lj::Wiper<T, D>

    /*!
     \brief Memory wiping implementation specialized for array objects.

     writes zeros to the provided memory area before releasing the memory. This
     implementation still needs some help as it doesn't understand how long the
     array is without being explicitly told. which requires some ugly code like
     \code
     std::unique_ptr<MyType[], Wiper<MyType[]>> array(new MyType[10]);
     array.get_deleter().set_count(10);
     \endcode

     \tparam T the type to be wiped.
     \tparam D the "delete" provider type.
     */
    template<typename T, typename D>
    class Wiper<T[], D>
    {
    public:
        //! Default constructor.
        Wiper() : count_(1)
        {
        }

        /*!
         \brief Functor operator.

         This primarily allows this class to act as a replacement for the default
         deleter on std::unique_ptr objects. This invokes the wipe and the
         delete.

         This will wipe \c count number of \c T objects.

         \c set_count() must be called first.
         \param t The pointer to wipe and delete.
         */
        void operator()(T* t) const
        {
            wipe(t, count_);
            D deleter;
            deleter(t);
        }

        /*!
         \brief wipe method.

         write zeros to a range of memory.

         \todo This needs to be checked to make sure compilers aren't
         optimizing it into a no-op.

         \param t The std::unique_ptr to an object to wipe.
         \param count The number of T objects to wipe.
         \*/
        static void wipe(T* t, const size_t count)
        {
            size_t sz = sizeof (T) * count;
            volatile uint8_t* p = ((uint8_t*) t) + sz;
            while (sz--)
            {
                *(--p) = 0;
            }
        }

        /*!
         \brief Helper method for dealing with std::unique_ptr objects.
         \param t A unique_ptr to an new[] array.
         \param count The number of T objects to wipe.
         */
        static inline void wipe(std::unique_ptr<T[]>& t, const size_t count)
        {
            wipe(t.get(), count);
        }

        /*!
         \brief Set the number of T objects to be wiped when the deleter is invoked.
         \param count The number of T objects to wipe.
         */
        void set_count(const size_t count)
        {
            count_ = count;
        }

        /*!
         \brief Get the number of T objects set to be wiped.
         \return The number of T objects to wipe.
         */
        inline size_t count() const
        {
            return count_;
        }
    private:
        size_t count_;
    }; // class lj::Wiper<T[], D>
}; // namespace lj
