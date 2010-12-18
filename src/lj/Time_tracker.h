#pragma once
/*!
 \file Time_tracker.h
 \brief LJ time tracking code.
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

#include <utility>

namespace lj
{
    //! Utility class to track elapsed time.
    class Time_tracker
    {
    public:
        //! Constructor.
        Time_tracker();
        
        //! Start tracking time.
        void start();
        
        //! Stop tracking time.
        /*!
         \return The amount of time elapsed.
         */
        unsigned long long stop();
        
        //! Get the elapsed time.
        /*!
         \return The amount of time elapsed.
         */
        unsigned long long elapsed() const;
    private:
        struct Point_in_time
        {
            unsigned long long seconds;
            unsigned long long useconds;
            Point_in_time(unsigned long long sec,
                          unsigned long long usec) : seconds(sec),
                                                     useconds(usec)
            {
            }
        };

        Point_in_time start_;
        unsigned long long elapsed_;
    }; // class Time_tracker
}; // namespace lj
