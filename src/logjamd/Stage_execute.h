#pragma once
/*!
 \file Stage_execute.h
 \brief Logjam server client command execution stage definition.
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

#include "logjamd/Stage.h"

namespace logjamd
{
    //! Client command processor.
    /*!
     \author Jason Watson
     \version 1.0
     \date October 26, 2010
     */
    class Stage_execute : public Stage
    {
    public:
        //! Default constructor.
        Stage_execute();

        //! Virtual destructor.
        virtual ~Stage_execute();

        //! Method representing the logic of the processor.
        /*!
         \par
         currently expects the following document:
         \code
         {
             lj__command='<lua script>'
         }
         \endcode
         \param request The request document to process.
         \return The processor to use for the next request.
         */
        virtual Stage* logic(lj::Bson& request, Connection& connection);

    private:
        //! Hidden copy constructor.
        /*!
         \param orig Original object.
         */
        Stage_execute(const Stage_execute& orig);
    };
};
