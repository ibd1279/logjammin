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
#include "lj/Log.h"

extern "C"
{
#include "pthread.h"
}

namespace lj
{
    //! Interface for threaded work.
    /*!
     \par
     Provides an interface for work. Used by lj::Thread to invoke and
     clean up a new threada.
     \author Jason Watson <jwatson@slashopt.net>
     \date September 12, 2011
     \sa lj::Thread
     */
    class Work
    {
    public:
        //! Logic to be performed in another thread.
        /*!
         \par
         The actual logic to be performed by a thread.
         */
        virtual void run() = 0;
        
        //! Thread clean up logic.
        /*!
         \par
         Invoked at the termination of the thread. This allows a thread
         to clean up resources when it is aborted, etc.
         \todo Should this be in the destructor instead of a new method?
         */
        virtual void cleanup() = 0;
    };

    //! Thin wrapper for pthread.
    /*!
     \par
     Apparently std::thread is not support on the Mac yet. This is a place
     holder until pthread/stdc++ adds the necessary components to make
     std::thread work there. atleast until it organically grows to the point
     where std::thread is not an option.
     \author Jason Watson <jwatson@slashopt.net>
     \date September 12, 2011
     \sa lj::Work
     */
    class Thread
    {
    public:
        //! Construct a new thread.
        Thread();

        //! Deleted constructor
        Thread(const Thread& orig) = delete;

        //! Destructor.
        /*!
         \par
         abort() is called on the thread if it is still running.
         */
        ~Thread();

        //! Test if the thread is still running.
        /*!
         \return true if the thrad is running, false otherwise.
         */
        inline bool running() const
        {
            return work_ != NULL;
        }

        //! Execute some work in the thread.
        /*!
         \par
         The work->run() methcd is invoked in another thread. Upon thread
         exit or abort, the work->cleanup() method is invoked.
         \par
         \c The caller is responsible for releasing the work pointer.
         \param work The work to perform.
         */
        void run(Work* work);

        //! Execute some work in the thread.
        /*!
         \par
         Creates work object based on the provided anonymous functions and
         and executes that work.
         \param r_fun The run lambda.
         \param c_fun The cleanup lambda.
         */
        template<class R, class C>
        void run(R r_fun, C c_fun)
        {
            run(new lj::Thread::Lambda_work<R, C>(r_fun, c_fun));
        }

        //! abort the running thread.
        /*!
         \par
         tells the running thread to cancel, then joins the thread.
         */
        void abort();

        //! Join the calling thread with the target thread.
        void join();

        template<class R, class C>
        class Lambda_work : public lj::Work
        {
        public:
            Lambda_work(R r,
                    C c) :
                    lj::Work(),
                    run_(r),
                    cleanup_(c)
            {
            }
            virtual ~Lambda_work() {
            }
            virtual void run()
            {
                return run_();
            }
            virtual void cleanup()
            {
                cleanup_();
                delete this;
            }
        private:
            R run_;
            C cleanup_;
        };

    private:
        static void* pthread_run(void* obj);

        static void pthread_cleanup(void* obj);

        pthread_t thread_;
        Work* volatile work_;

    };
};

