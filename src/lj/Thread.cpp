/*!
 \file lj/Thread.cpp
 \brief LJ Thread implementation.
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

#include "lj/Thread.h"

#include <iostream>

namespace lj
{
    Future::Future(lj::Thread* thread) : complete_(false), result_(NULL), thread_(thread)
    {
    }

    bool Future::complete() const
    {
        return complete_;
    }

    Thread::Thread() : thread_(), result_(NULL), work_(NULL)
    {
    }
    
    Thread::~Thread()
    {
        join();
    }

    bool Thread::running() const
    {
        return work_ != NULL;
    }

    void Thread::run(lj::Work* work)
    {
        if (running())
        {
            throw LJ__Exception("Thread has already been assigned work.");
        }
        work_ = work;
        pthread_create(&thread_, NULL, &lj::Thread::pthread_run, this);
    }

    lj::Future* Thread::call(lj::Work* work)
    {
        if (running())
        {
            throw LJ__Exception("Thread has already been assigned work.");
        }
        result_ = new lj::Future(this);
        work_ = work;
        pthread_create(&thread_, NULL, &lj::Thread::pthread_call, this);
        return result_;
    }

    void Thread::abort()
    {
        if (running())
        {
            work_->abort();
            pthread_join(thread_, NULL);
        }
    }

    void Thread::join()
    {
        pthread_join(thread_, NULL);
    }

    void Thread::run_entry()
    {
        work_->run();
        lj::Work* work = work_;
        work_ = NULL;
        delete work;
    }

    void* Thread::call_entry()
    {
        void* result = work_->call();
        lj::Work* work = work_;
        work_ = NULL;
        delete work;
        return result;
    }

    void* Thread::pthread_run(void* obj)
    {
        reinterpret_cast<lj::Thread*>(obj)->run_entry();
        return NULL;
    }

    void* Thread::pthread_call(void* obj)
    {
        lj::Thread* ptr = reinterpret_cast<lj::Thread*>(obj);
        ptr->result_->result_ = ptr->call_entry();
        ptr->result_->complete_ = true;
        return NULL;
    }
};
