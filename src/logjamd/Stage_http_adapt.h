#pragma once
/*!
 \file Stage_http_adapt.h
 \brief Logjam server stage for converting http json input into bson input.
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

#include "logjam/Stage.h"

namespace logjamd
{
    //! HTTP to bson adaptor stage.
    class Stage_http_adapt : public logjam::Stage
    {
    public:
        Stage_http_adapt() = default;
        Stage_http_adapt(const Stage_http_adapt& o) = default;
        Stage_http_adapt(Stage_http_adapt&& o) = default;
        Stage_http_adapt& operator=(const Stage_http_adapt& rhs) = default;
        Stage_http_adapt& operator=(Stage_http_adapt&& rhs) = default;
        virtual ~Stage_http_adapt() = default;
        virtual std::unique_ptr<logjam::Stage> logic(
                logjam::pool::Swimmer& swmr) const override;
        virtual std::string name() const override;
        virtual std::unique_ptr<logjam::Stage> clone() const override;
    };
};

