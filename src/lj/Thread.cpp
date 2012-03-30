/*!
 \file lj/Thread.cpp
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

#include "lj/Thread.h"
#include "lj/Log.h"
#include <cassert>

namespace lj
{
    Thread::Thread() : thread_(), work_(NULL)
    {
    }

    Thread::~Thread()
    {
        if (running())
        {
            log::format<Notice>("Aborting thread from thread destructor.").end();
            abort();
        }
    }

    void Thread::run(Work* work)
    {
        if (running())
        {
            throw LJ__Exception("Thread has already been assigned work.");
        }

        work_ = work;
        pthread_create(&thread_, nullptr, &lj::Thread::pthread_run, this);
    }

    void Thread::abort()
    {
        if (running())
        {
            pthread_cancel(thread_);
            join();
        }
    }

    void Thread::join()
    {
        if (running())
        {
            pthread_join(thread_, nullptr);
        }
    }

    void* Thread::pthread_run(void* obj)
    {
        // convert the passed object back into a thread instance.
        lj::Thread* ptr = static_cast<lj::Thread*>(obj);

        // Ensure cleanup gets called.
        pthread_cleanup_push(lj::Thread::pthread_cleanup, obj);

        // Attempt to perform some work.
        log::attempt<Critical>([ptr] { ptr->work_->run(); });

        // Force cleanup to get called.
        pthread_cleanup_pop(1);

        // pthread return value.
        // TODO this should probably do something more useful and store the
        // TODO result on the work object.
        return nullptr;
    }

    void Thread::pthread_cleanup(void* obj)
    {
        // XXX I feel like this might be dangerous if a thread is reused.

        // convert the passed object back into a thread instance.
        lj::Thread* ptr = static_cast<lj::Thread*>(obj);

        // mark the thread as no longer running.
        Work* work = ptr->work_;
        ptr->work_ = nullptr;

        // attempt to perform the cleanup code.
        log::attempt<Critical>([work] { work->cleanup(); });
    }
};

