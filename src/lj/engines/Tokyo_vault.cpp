/*!
 \file lj/engines/Tokyo_vault.cpp
 \brief LJ Tokyo Vault Storage engine implementation.
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

#include "lj/Tokyo_vault.h"

namespace lj
{
    namespace engines
    {
        Tokyo_vault::Tokyo_vault(const lj::Storage* const storage,
                                 const lj::Bson* const config) :
                                 lj::Vault(storage)
        {
        }
        
        Tokyo_vault::~Tokyo_vault()
        {
        }

        void Tokyo_vault::place(lj::Bson& item)
        {
            lj::Uuid& uid = lj::bson_as_uuid(item["__uid"]);
            size_t sz;
            const uint8_t* const key = uid.data(&sz);
            data_->place(key
                         sz,
                         item,
                         item.size());
        }

        void Tokyo_vault::remove(lj::Bson& item)
        {
            lj::Uuid& uid = lj::bson_as_uuid(item["__uid"]);
            size_t sz;
            const uint8_t* const key = uid.data(&sz);
            data_->remove(&key,
                          sz);
        }

        void Tokyo_vault::journal_begin(const lj::Uuid& uid)
        {
            bool complete = false;
            size_t sz;
            const uint8_t* const key = uid.data(&sz);
            journal_->start_writes();
            journal_->place(key,
                            sz,
                            &complete,
                            sizeof(bool));
            journal_->end_writes();
        }

        void Tokyo_vault::journal_end(const lj::Uuid& uid)
        {
            bool complete = true;
            size_t sz;
            const uint8_t* const key = uid.data(&sz);
            journal_->start_writes();
            journal_->place(key,
                            sz,
                            &complete,
                            sizeof(bool));
            journal_->end_writes();
        }

        uint64_t Tokyo_vault::size()
        {
        }

        bool Tokyo_vault::items(const lj::Index* const index,
                                std::list<Bson>& records) const
            
        {
        }

        bool Tokyo_vault::items(const lj::Index* const index,
                                std::list<Bson*>& records) const
        {
        }

        bool Tokyo_vault::items_raw(const lj::Index* const index,
                                    lj::Bson& records) const
        {
        }

        bool Tokyo_vault::first(const lj::Index* const index,
                                lj::Bson& result) const
        {
        }
    }; // namespace lj::engines
}; // namespace lj.
