/*!
 \file Stage.cpp
 \brief Logjam server stage abstract base source.
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

#include "logjam/Stage.h"

namespace logjam
{
    lj::log::Logger& Stage::log(const std::string& fmt) const
    {
        std::string real_fmt("%s: ");
        real_fmt.append(fmt);
        return lj::log::format<lj::Debug>(real_fmt) << name();
    }

    std::unique_ptr<Stage> safe_execute_stage(std::unique_ptr<Stage>& stg,
            pool::Swimmer& swmr)
    {
        std::unique_ptr<Stage> result(nullptr);
        if (nullptr != stg)
        {
            result = stg->logic(swmr);
            if (result.get() == stg.get())
            {
                // Release the result pointer, because we don't
                // want the stack unwind to release the pointer twice.
                result.release();
                std::ostringstream oss;
                oss << "Stage Logic Error. " << stg->name()
                        << " logic() returned itself as the next stage."
                        << " The next stage must be null or a new pointer."
                        << " Never an existing Stage object.";
                throw LJ__Exception(oss.str());
            }
        }

        return result;
    }

    /*
    // All this needs to find a new home in logjamd
    std::unique_ptr<Stage> new_pre_stage(logjam::pool::Swimmer& swimmer)
    {
        return std::unique_ptr<Stage>(new Stage_pre(swimmer));
    }

    std::unique_ptr<Stage> new_first_stage(logjam::pool::Swimmer& swimmer)
    {
        return std::unique_ptr<Stage>(new Stage_auth(swimmer, 0));
    }
    */
};

