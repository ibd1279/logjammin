#pragma once
/*!
 \file Stage.h
 \brief Logjam stage abstract base definition.
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

#include "logjam/Pool.h"
#include "lj/Bson.h"
#include "lj/Log.h"
#include <memory>
#include <string>

namespace logjam
{
    //! A stage is a specific server execution unit.
    /*!
     When establishing and executing different connections, the client and the
     server go through different stages. The first stage is the pre-stage. It
     involves things like the TLS handshake, selecting the communication method,
     etc. After that comes authentication, and then things start to diverge
     based on the handshake result.

     It is expected that Stage objects are stateless, and immutable. Any
     state information should be attached to the swimmer context, and not
     added as instance state.
     \since 0.1
     */
    class Stage
    {
    public:
        Stage() = default;
        Stage(const Stage& o) = default;
        Stage(Stage&& o) = default;
        Stage& operator=(const Stage& rhs) = default;
        Stage& operator=(Stage&& rhs) = default;
        virtual ~Stage() = default;
        virtual std::unique_ptr<Stage> logic(pool::Swimmer& swmr) const = 0;
        virtual std::string name() const = 0;
        virtual std::unique_ptr<Stage> clone() const = 0;
    protected:
        virtual lj::log::Logger& log(const std::string& fmt) const;
    }; // class logjam::Stage

    //! Safely execute one stage and return another stage.
    /*!
     performs checks to make sure a stage does not return itself.
     \param stg The stage to execute.
     \param swmr The swimmer to use while executing the stage.
     \return The new stage.
     \throws lj::Exception if the stage attempts to return itself.
     */
    std::unique_ptr<Stage> safe_execute_stage(std::unique_ptr<Stage>& stg,
            pool::Swimmer& swmr);
};
