#pragma once
/*!
 \file Client_processor.h
 \brief Logjam server client definition.
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

#include "logjamd/Connection.h"
#include "lj/Bson.h"

namespace logjamd
{
    //! Client processor base class.
    /*!
     \par
     Base class represents the interface used by different stages in the
     client connection.
     \par
     The Client_processor::logic(lj::Bson&) method must be implemented by concrete implementations.
     \author Jason Watson
     \version 1.0
     \date October 26, 2010
     */
    class Client_processor
    {
    public:
        //! Method representing the logic of the processor.
        /*!
         \param request The request document to process.
         \param connection The client connection
         \return The processor to use for the next request. Null to terminate
         the connection.
         */
        virtual Client_processor* logic(lj::Bson& request, Connection& connection) = 0;
    protected:
        //! Default constructor.
        Client_processor();
        //! Virtual destructor.
        virtual ~Client_processor();
    private:
        //! Hidden copy constructor.
        /*!
         \param orig Original object.
         */
        Client_processor(const Client_processor& orig);
    };
};
