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

#include <list>
#include <string>
#include <sstream>
#include "Document.h"
#include "Tokyo.h"
#include "Storage.h"

namespace logjammin {
    
    //! Baseclass for data storage.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    class Model {
    protected:
        tokyo::Document _d;
        
        //! Create and open the DB object.
        Model() : _d() {
        }
        
        //! Copy constructor.
        /*!
         \param orig The orignal model to copy.
         */
        Model(const Model &orig) : _d(orig._d) {
        }
        
        Model(const tokyo::Document &d) : _d(d) {
        }
        
        //! Close the database object.
        virtual ~Model() {
        }
        
        //! Get the DAO
        virtual tokyo::Storage *dao() const = 0;
    public:
        //! Save the current object into the database.
        virtual void save() {
            dao()->place(_d);
        }
        
        //! Remove the current object from the database.
        virtual void purge() {
            dao()->remove(_d);
        }
        
        tokyo::Document doc() { return _d; }
        
        //! Return a document node for the given path.
        virtual const tokyo::DocumentNode &field(const std::string &path) const {
            return _d.path(path);
        }
        
        virtual const tokyo::DocumentNode &operator[](const std::string &path) const {
            return field(path);
        }
        
        virtual Model &field(const std::string &path, const std::string &value) {
            _d.path(path, value);
            return *this;
        }
        virtual Model &field(const std::string &path, const long long value) {
            _d.path(path, value);
            return *this;
        }
        virtual Model &field(const std::string &path, const int &value) {
            _d.path(path, value);
            return *this;
        }
        virtual Model &field(const std::string &path, const double &value) {
            _d.path(path, value);
            return *this;
        }
        
        //! Get the primary key for the current object.
        unsigned long long pkey() const { return _d.key(); };
    };
}; // namespace logjammin

