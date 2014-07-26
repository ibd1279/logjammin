#pragma once
/*!
 \file lj/Threaded_queue.h
 \brief LJ Thread Safe Queue Header.
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

#include <condition_variable>
#include <mutex>
#include <queue>

namespace lj
{
    //! Thread safe queue.
    /*!
     \tparam T The type of the queue.
     */
    template <class T>
    class Threaded_queue
    {
    public:
        //! Create a new empty threaded queue.
        Threaded_queue() {}

        //! Deleted copy constructor.
        Threaded_queue(const Threaded_queue& orig) = delete;

        //! Move constructor.
        Threaded_queue(Threaded_queue&& orig) = default;
        
        //! Deleted copy assignment operator.
        Threaded_queue& operator=(const Threaded_queue& orig) = delete;

        //! Move assignment operator.
        Threaded_queue& operator=(Threaded_queue&& orig) = default;

        //! Push an object onto the queue.
        void push(const T& item)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push(item);
            lock.unlock();
            cv_.notify_one();
        }

        //! Pop an object off the queue.
        T pop()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this](){return this->queue_.size() > 0;});
            T n(queue_.front());
            queue_.pop();
            return n;
        }
    private:
        std::mutex mutex_;
        std::queue<T> queue_;
        std::condition_variable cv_;
    }; // namespace lj::Threaded_queue
}; // namespace lj
