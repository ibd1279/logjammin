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
#include "openprop.h"
#include "ModelDB.h"

namespace logjammin {
    
    //! Baseclass for data storage.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<class V>
    class Model {
        friend class ModelDB<V>;
    public:
        //! Save the current object into the database.
        virtual void save() {
            V *model = dynamic_cast<V *>(this);
            if(model == NULL)
                throw std::string("Invalid combination of Model and ModelDB.");
            
            dao()->put(model);
        }
        
        //! Remove the current object from the database.
        virtual void purge() {
            V *model = dynamic_cast<V *>(this);
            if(model == NULL)
                throw std::string("Invalid combination of Model and ModelDB.");
            
            dao()->remove(model);
        }
        
        //! Get the primary key for the current object.
        /*!
         The primary key should be positive for records that exist in the database.
         Zero for records that do not exist in the database.
         \return The primary key for the object.
         */
        virtual unsigned long long pkey() const { return _pkey; }
        
        //! Get the serialized version of this instance.
        /*!
         Instances should override this method to serialize the fields of the object.
         The output format should be in OpenProp format.
         \return The OpenProp string representing this object.
         */
        virtual const std::string serialize() const = 0;
        
        //! Convert a database record into an instance object.
        /*!
         \param props A structured object to load values from.
         \sa load().
         */
        virtual void populate(OpenProp::File *props) = 0;

        //! Helper method to escape strings for OpenProp format.
        /*!
         \param val The value to escape.
         \return The escaped version.
         \sa populate() and serialize().
         */
        static std::string escape(const std::string &val) {
            std::string::const_iterator iter = val.begin();
            std::string r;
            for(; iter != val.end(); ++iter) {
                char c = *iter;
                if(c == '\\' || c == '"')
                    r.push_back('\\');
                else if(c == '\n')
                    r.append("\\n\\");
                r.push_back(*iter);
            }
            return r;
        }
                
    protected:
        //! Set the primary key for the current object.
        /*!
         This method should only be called when populating, purging, or saving.
         \param key The primary key.
         */
        virtual void pkey(const unsigned long long key) { _pkey = key; }
        
        //! Get the DAO
        virtual ModelDB<V> *dao() const = 0;
        
        //! Create and open the DB object.
        Model() {
            _pkey = 0;
        }
        
        //! Copy constructor.
        /*!
         \param orig The orignal model to copy.
         */
        Model(const Model &orig) : _pkey(orig._pkey) {
        }
        
        //! Close the database object.
        virtual ~Model() {
        }
        
        //! Current primary key.
        unsigned long long _pkey;
    };
}; // namespace logjammin

