/*!
 \file Record_set.cpp
 \brief LJ Record_set implementation.
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

#include "lj/Time_tracker.h"

#include <sys/time.h>

namespace lj
{
    void Time_tracker::start()
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        start_.first = now.tv_sec;
        start_.second = now.tv_usec;
    }
    
    unsigned long long Time_tracker::stop()
    {
        if (start_.first)
        {
            elapsed_ += elapsed();
            start_.first = 0;
            start_.second = 0;
        }
        return elapsed_;
    }
    
    unsigned long long Time_tracker::elapsed() const
    {
        if (start_.first)
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            std::pair<unsigned long long, unsigned long long> end(now.tv_sec, now.tv_usec);
            return (((end.first - start_.first) * 1000000UL) +
                    (end.second - start_.second));
        }
        else
        {
            return elapsed_;
        }
    }
        
}; // namespace lj