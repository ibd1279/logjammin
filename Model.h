#pragma once
/*
 \file Model.h
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

#include "BSONNode.h"
#include "Storage.h"

namespace logjammin {
    
    //! Baseclass for data storage.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    class Model : public lj::BSONNode {
    protected:
        //! Create and open the DB object.
        Model() : lj::BSONNode() {
        }
        
        //! Copy constructor.
        /*!
         \param orig The orignal model to copy.
         */
        Model(const Model &orig) : lj::BSONNode(orig) {
        }
        
        //! Close the database object.
        virtual ~Model() {
        }
        
        //! Get the DAO
        virtual lj::Storage *dao() const = 0;
    public:
        //! Save the current object into the database.
        virtual void save() {
            dao()->place(*this);
        }
        
        //! Remove the current object from the database.
        virtual void purge() {
            dao()->remove(*this);
        }
        
        //! Get the primary key for the current object.
        inline unsigned long long pkey() const { return nav("__key").to_l(); };
    };
}; // namespace logjammin

