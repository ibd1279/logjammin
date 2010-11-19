#pragma once
/*!
 \file Standard_record_set.h
 \brief LJ Standard_record_set header.
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

#include "lj/Record_set.h"

namespace lj
{
    class Storage;
    
    //! Represents a set of records.
    /*!
     \par
     This represents a set of records identified by ids.
     \sa lj::All_record_set
     \sa lj::Record_set
     */
    class Standard_record_set : public Record_set {
    public:
        //! Set-copy constructor.
        /*!
         Create a new Standard_record_set, copying values from the provided
         set of keys.
         \param storage The storage this set is attached to.
         \param keys The set of keys to copy into the set.
         \param op The operation to perform for any filtering.
         */
        Standard_record_set(const Storage* storage,
                            const std::set<unsigned long long>& keys,
                            const Record_set::Operation op);
        
        //! Set-owner constructor.
        /*!
         Create a new Standard_record_set, taking ownership of the provided
         key set.
         \param storage The storage this set is attached to.
         \param keys The set of keys included in the record set.
         \param op The operation to perform for any filtering.
         */
        Standard_record_set(const Storage* storage,
                            std::set<unsigned long long>* keys,
                            const Record_set::Operation op);
        
        //! Copy constructor.
        /*!
         \param orig The original.
         */
        Standard_record_set(const Standard_record_set& orig);
        
        //! Destructor.
        virtual ~Standard_record_set();
        virtual Record_set& set_operation(const Record_set::Operation op);
        virtual bool is_included(const unsigned long long key) const;
        virtual std::unique_ptr<Record_set> include_keys(const std::set<unsigned long long>& keys);
        virtual std::unique_ptr<Record_set> include_key(const unsigned long long key);
        virtual std::unique_ptr<Record_set> exclude_keys(const std::set<unsigned long long> &keys);
        virtual std::unique_ptr<Record_set> exclude_key(const unsigned long long key);
        virtual std::unique_ptr<Record_set> equal(const std::string& indx,
                                                const void* const val,
                                                const size_t len) const;
        virtual std::unique_ptr<Record_set> greater(const std::string& indx,
                                                  const void* const val,
                                                  const size_t len) const;
        virtual std::unique_ptr<Record_set> lesser(const std::string& indx,
                                                 const void* const val,
                                                 const size_t len) const;
        virtual std::unique_ptr<Record_set> contains(const std::string& indx,
                                                   const std::string& term) const;
        virtual std::unique_ptr<Record_set> tagged(const std::string& indx,
                                                 const std::string& word) const;
        virtual long long size() const;
        virtual bool items(std::list<Bson>& records) const;
        virtual bool items(std::list<Bson*>& records) const;
        virtual bool first(Bson& result) const;
        virtual bool items_raw(lj::Bson& records) const;
        virtual void set_raw_size(long long sz);
        virtual long long raw_size() const;
        virtual const lj::Storage& storage() const;
    private:
        const Storage *storage_;
        std::set<unsigned long long>* keys_;
        Record_set::Operation op_;
        long long raw_size_;
        
        //! Hidden.
        Record_set& operator=(const Standard_record_set& o);
        
        //! Get a record from the database.
        /*!
         \param pkey The key of the record.
         \param marshall True to parse the record into a full structure. False
         to leave the records as an array of bytes.
         */
        std::unique_ptr<Bson> doc_at(unsigned long long pkey, bool marshall) const;
    };
}; // namespace lj
