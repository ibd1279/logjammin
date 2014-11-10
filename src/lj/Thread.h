#pragma once
/*!
 \file lj/Thread.h
 \brief LJ Thread header.
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

#include "lj/Exception.h"
#include "lj/Log.h"

extern "C"
{
#include "pthread.h"
}

namespace lj
{
    /*!
     \brief Interface for threaded work.

     Provides an interface for work. Used by lj::Thread to invoke and
     clean up a new threads. This is mostly just a wrapper on pthread.

     \since 1.0
     \sa lj::Thread
     */
    class Work
    {
    public:
        /*!
         \brief Logic to be performed in another thread.

         The actual logic to be performed by a thread.
         */
        virtual void run() = 0;

        /*!
         \brief Thread clean up logic.

         Invoked at the termination of the thread. This allows a thread
         to clean up resources when it is aborted, etc.

         \todo Should this be in the destructor instead of a new method?
         */
        virtual void cleanup() = 0;
    };

    /*!
     \brief Thin wrapper for pthread.

     Apparently std::thread is not supported on the Mac yet. This is a place
     holder until pthread/stdc++ adds the necessary components to make
     std::thread work there. atleast until it organically grows to the point
     where std::thread is not an option.

     \todo This should be re-evaulated now that the code base has been switched
     over to use clang libc++ instead of the gnu libraries. STL threads are
     there, my time just hasn't been.

     \since 1.0
     \sa lj::Work
     */
    class Thread
    {
    public:
        //! Construct a new thread.
        Thread();

        /*!
         \brief Deleted copy constructor
         \param orig The original Object.
         */
        Thread(const Thread& orig) = delete;

        /*!
         \brief Deleted move constructor
         \param orig The original object.
         */
        Thread(Thread&& orig) = delete;

        /*!
         \brief Destructor.

         abort() is called on the thread if it is still running.
         */
        ~Thread();

        /*!
         \brief Deleted copy assignment operator.
         \param orig The original object.
         \return A reference to this.
         */
        Thread& operator=(const Thread& orig) = delete;

        /*!
         \brief Deleted move assignment operator.
         \param orig The original object.
         \return A reference to this.
         */
        Thread& operator=(Thread&& orig) = delete;

        /*!
         \brief Test if the thread is still running.
         \return true if the thrad is running, false otherwise.
         */
        inline bool running() const
        {
            return work_ != NULL;
        }

        /*!
         \brief Execute some work in the thread.

         The work->run() methcd is invoked in another thread. Upon thread
         exit or abort, the work->cleanup() method is invoked.

         \c The caller is responsible for releasing the work pointer.
         \param work The work to perform.
         */
        void run(Work* work);

        /*!
         \brief Execute some work in the thread.

         Creates work object based on the provided anonymous functions and
         and executes that work.

         \tparam R the type of the run lambda
         \tparam C the type of the cleanup lambda
         \param r_fun The run lambda.
         \param c_fun The cleanup lambda.
         */
        template<class R, class C>
        void run(R r_fun, C c_fun)
        {
            run(new lj::Thread::Lambda_work<R, C>(r_fun, c_fun));
        }

        /*!
         \brief abort the running thread.

         tells the running thread to cancel, then joins the thread.
         */
        void abort();

        //! Join the calling thread with the target thread.
        void join();

        /*!
         \brief Wraps two functions in an object for usage with a thread.

         In some cases, you may wish to provide two functions for executing a
         thread instead of a specific class. \c Lambda_work is specifically
         designed to wrap two no-argument functions into a lj::Work object.

         \sa lj::Work
         \tparam R The run function pointer type. Must not require any arguments.
         \tparam C The cleanup function pointer type. Must not require any arguments.
         */
        template<class R, class C>
        class Lambda_work : public lj::Work
        {
        public:
            /*!
             \brief Constructor
             \param r The run function pointer.
             \param c The cleanup function pointer.
             */
            Lambda_work(R r,
                    C c) :
                    lj::Work(),
                    run_(r),
                    cleanup_(c)
            {
            }

            /*!
             \brief Copy constructor.
             \param orig The original object.
             */
            Lambda_work(const Lambda_work<R, C>& orig) :
                    lj::Work(),
                    run_(orig.run_),
                    cleanup_(orig.cleanup_)
            {
            }

            /*!
             \brief Move constructor.
             \param orig The original object.
             */
            Lambda_work(Lambda_work<R, C>&& orig) :
                    lj::Work(),
                    run_(orig.run_),
                    cleanup_(orig.cleanup_)
            {
            }

            /*!
             \brief Destructor.

             Params are expected to be pointers to functions. Nothing to delete.
             When an object based usecase is created, it can be spun off as a
             specialized template
             */
            virtual ~Lambda_work() {
            }

            /*!
             \brief Copy assignment operator.
             \param orig The original object.
             \return A reference to this.
             */
            Lambda_work<R, C>& operator=(const Lambda_work<R, C>& orig)
            {
                run_ = orig.run_;
                cleanup_ = orig.cleanup_;
                return *this;
            }

            /*!
             \brief Move assignment operator.
             \param orig The original object.
             \return A reference to this.
             */
            Lambda_work<R, C>& operator=(Lambda_work<R, C>&& orig)
            {
                run_ = orig.run_;
                cleanup_ = orig.cleanup_;
                return *this;
            }

            /*!
             \brief Invoke the run function pointer.
             \sa lj::Work::run()
             */
            virtual void run() override
            {
                run_();
            }

            /*!
             \brief Invoke the cleanup function pointer.
             \sa lj::Work::cleanup()
             */
            virtual void cleanup() override
            {
                cleanup_();
                delete this;
            }
        private:
            R run_;
            C cleanup_;
        }; // class lj::Thread::Lambda_work

    private:
        static void* pthread_run(void* obj); //!< bridge function between pthread and work.
        static void pthread_cleanup(void* obj); //!< bridge function between pthread and work.

        pthread_t thread_;
        Work* volatile work_;
    }; // class lj::Thread
}; // namespace lj

