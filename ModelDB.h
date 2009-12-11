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
#include "Tokyo.h"

namespace logjammin {
    
    //! Base Class for Object/Relational Mapping.
    /*!
     Provides a base for implementing more complicated model DB objects.
     Methods in this class should be overridden to perform additional
     commands necessary for maintaining index integrity.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<class V>
    class ModelDB : public tokyo::DB<unsigned long long, std::string> {
    public:
        //! Create a new model DB object.
        /*!
         \param db_open_func Method for opening the database.
         \param mode Mode to open the database in.
         */
        ModelDB(void (*db_open_func)(TCBDB *, int), int mode) : tokyo::DB<unsigned long long, std::string>(db_open_func, mode) {
        }
        
        //! Store a record in the database.
        /*!
         Replaces the existing record if it alrady exists.
         \param model the object to store.
         \exception TokyoException When the database cannot be written.
         */
        virtual void put(V *model) = 0;
        
        //! Get a record from the database.
        /*!
         \param key The primary key of the object to get.
         \param model The object to populate with data.
         \exception TokyoException When the database cannot be read.
         */
        virtual void at(const unsigned long long key, V *model) {
            // Get data and turn it into a stream.
            std::string v = _at(key);
            std::istringstream data(v);
            
            // Convert the stream into a map.
            OpenProp::File *record = OpenProp::File::Load(data);
            if(!record)
                throw std::string("Unable to parse DB record");
            
            // Populate the current instance with the map.
            model->populate(record);
            delete record;
            
            set_pkey(model, key);
        }
        
        //! Get all the records from the database.
        /*!
         \param results List to place the results into.
         */
        virtual void all(std::list<V *> &results) {
            BDBCUR *cur = begin();
            
            do {
                std::string v = cursor_value(cur);
                std::istringstream data(v);
                
                // Convert the stream into a map.
                OpenProp::File *record = OpenProp::File::Load(data);
                if(!record)
                    throw std::string("Unable to parse DB record");
                
                // Populate the current instance with the map.
                V *model = new V();
                model->populate(record);
                delete record;
                
                set_pkey(model, cursor_key(cur));
                results.push_back(model);
            } while(tcbdbcurnext(cur));
            tcbdbcurdel(cur);
        }
        
        //! Remove a record from the database.
        /*!
         \param model The model object to remove.
         \exception TokyoException when the database cannot be written.
         */
        virtual void remove(V *model) = 0;
        
    protected:
        //! Method for setting the primary key.
        /*!
         The primary key cannot be directly set by inherited classes. This
         method gets around that limitation.
         \param model The model to set the primary key on.
         \param key The key value.
         */
        void set_pkey(V* model, unsigned long long key) {
            model->pkey(key);
        }
    };
};