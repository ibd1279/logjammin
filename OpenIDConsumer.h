#pragma once
/*
 \file OpenIDConsumer.h
 \author Jason Watson
 Copyright (c) 2009, Jason Watson
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

#include "OpenID.h"

namespace logjammin {
    
    //! Logjammin OpenID Consumer.
    /*!
     \par
     Specific implementation for Logjammin that uses the Tokyo libraries for
     storing state information associated with providers.
     \author Jason Watson
     \version 1.0
     \date July 29, 2009
     */
    class OpenIDConsumer : public openid_1_1::AssociatedRelayConsumer {
    public:
        //! Create a new Logjammin' consumer.
        /*!
         \param identifier The user provided identifier.
         */
        OpenIDConsumer(const std::string &identifier);
        
        //! Logjammin' destructor.
        virtual ~OpenIDConsumer();
        virtual void invalidate_assoc_handle(const std::string &assoc_handle);
        virtual const std::string *lookup_assoc_handle(const std::string &provider);
        virtual openid_1_1::Association *lookup_association(const std::string &assoc_handle);
        virtual void store_assoc_handle(const openid_1_1::Association *association);
    };
    
};