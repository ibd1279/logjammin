#pragma once
/*!
 \file lj/Thread.h
 \brief LJ Thread header.
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
#include <pthread.h>

namespace lj
{
    class Future;

    class Work
    {
    public:
        virtual void run() = 0;
        virtual void* call() = 0;
        virtual void abort() = 0;
    };

    class Thread
    {
    public:
        Thread();
        Thread(const Thread& orig) = delete;
        ~Thread();
        bool running() const;

        void run(lj::Work* work);

        lj::Future* call(lj::Work* work);

        void abort();

        void join();
    private:
        void run_entry();

        void* call_entry();

        static void* pthread_run(void* obj);

        static void* pthread_call(void* obj);

        pthread_t thread_;
        lj::Future* result_;
        lj::Work* volatile work_;
    };

    class Future
    {
    public:
        friend class Thread;
        Future(lj::Thread* thread);
        bool complete() const;
        template<class T>
        T* result()
        {
            if (!complete())
            {
                thread_->join();
            }
            return reinterpret_cast<T*>(result_);
        }
    private:
        volatile bool complete_;
        void* volatile result_;
        lj::Thread* thread_;
    };
};
