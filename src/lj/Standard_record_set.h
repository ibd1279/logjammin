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

#include "lj/Logger.h"

namespace lj
{
    class Storage;
    
    class Standard_record_set : public Record_set {
    public:
        Standard_record_set(const Storage* storage,
                            const std::set<unsigned long long>& keys,
                            const Record_set::Operation op);
        Standard_record_set(const Storage* storage,
                            std::set<unsigned long long>* keys,
                            const Record_set::Operation op);
        Standard_record_set(const Standard_record_set& orig);
        virtual ~Standard_record_set();
        virtual Record_set& set_operation(const Record_set::Operation op);
        virtual bool is_included(const unsigned long long key) const;
        virtual std::auto_ptr<Record_set> include_keys(const std::set<unsigned long long>& keys);
        virtual std::auto_ptr<Record_set> include_key(const unsigned long long key);
        virtual std::auto_ptr<Record_set> exclude_keys(const std::set<unsigned long long> &keys);
        virtual std::auto_ptr<Record_set> exclude_key(const unsigned long long key);
        virtual std::auto_ptr<Record_set> equal(const std::string& indx,
                                                const void* const val,
                                                const size_t len) const;
        virtual std::auto_ptr<Record_set> greater(const std::string& indx,
                                                  const void* const val,
                                                  const size_t len) const;
        virtual std::auto_ptr<Record_set> lesser(const std::string& indx,
                                                 const void* const val,
                                                 const size_t len) const;
        virtual std::auto_ptr<Record_set> contains(const std::string& indx,
                                                   const std::string& term) const;
        virtual std::auto_ptr<Record_set> tagged(const std::string& indx,
                                                 const std::string& word) const;
        virtual unsigned long long size() const;
        virtual bool items(std::list<Bson>& records) const;
        virtual bool items(std::list<Bson*>& records) const;
        virtual bool first(Bson& result) const;
        virtual bool items_raw(std::list<Bson*>& records) const;
    private:
        const Storage *storage_;
        std::set<unsigned long long>* keys_;
        Record_set::Operation op_;
        Record_set& operator=(const Standard_record_set& o);
        std::auto_ptr<Bson> doc_at(unsigned long long pkey, bool marshall) const;
    };
}; // namespace lj