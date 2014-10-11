#pragma once
/*!
 \file logjamd/Stage_auth.h
 \brief Logjam server stage authentication header.
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

#include <cstdint>

namespace logjamd
{
    //! Implementation of the authentication stage for the logjamd server.
    /*!
     This stage reads the authentication information from the connection and
     passes the information to the authentication provider and selected
     method.
     \since 0.1
     */
    class Stage_auth : public logjam::Stage {
    public:
        Stage_auth() = default;
        Stage_auth(const Stage_auth& o) = default;
        Stage_auth(Stage_auth&& o) = default;
        Stage_auth& operator=(const Stage_auth& rhs) = default;
        Stage_auth& operator=(Stage_auth&& rhs) = default;
        virtual ~Stage_auth() = default;
        virtual std::unique_ptr<logjam::Stage> logic(
                logjam::pool::Swimmer& swmr) const override;
        virtual std::string name() const override;
        virtual std::unique_ptr<logjam::Stage> clone() const override;
    }; // class logjamd::Stage_auth
}; // namespace logjamd
