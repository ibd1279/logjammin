/*!
 \file lj/Engine.cpp
 \brief LJ Storage engine implementation.
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

#include "lj/Engine.h"
#include "lj/Storage.h"

#include <algorithm>

namespace lj
{
    Index::Index(const lj::Storage* const storage) : storage_(storage)
    {
    }

    Index::Index(const Index* const orig) : storage_(orig->storage_)
    {
    }
        
    Index::~Index()
    {
    }

    std::unique_ptr<Index> Index::eq(const std::string& indx,
                                     const void* const val,
                                     const size_t len,
                                     const MergeMode mode) const
    {
        return storage()->index(indx)->equal(val, len)->merge(mode, this);
    }
        
    std::unique_ptr<Index> Index::gt(const std::string& indx,
                                     const void* const val,
                                     const size_t len,
                                     const MergeMode mode) const
    {
        return storage()->index(indx)->greater(val, len)->merge(mode, this);
    }

    std::unique_ptr<Index> Index::lt(const std::string& indx,
                                     const void* const val,
                                     const size_t len,
                                     const MergeMode mode) const
    {
        return storage()->index(indx)->lesser(val, len)->merge(mode, this);
    }

    std::unique_ptr<Index> Index::merge(const MergeMode mode,
                                        const Index* const other)
    {
        const std::set<Uuid>& small = (this->size() < other->size()) ? this->keys() : other->keys();
        const std::set<Uuid>& big = (this->size() < other->size()) ? other->keys() : this->keys();
        Index* ret = this->clone();
        switch (mode)
        {
            case MergeMode::k_intersection:
                std::for_each(small.begin(), small.end(), [&ret, &big](const Uuid& uid)
                {
                    if (big.end() != big.find(uid))
                    {
                        ret->insert(uid);
                    }
                });
                break;
            case MergeMode::k_union:
                std::for_each(big.begin(), big.end(), [&ret] (const Uuid& uid)
                {
                    ret->insert(uid);
                });
                std::for_each(small.begin(), small.end(), [&ret] (const Uuid& uid)
                {
                    ret->insert(uid);
                });
                break;
            case MergeMode::k_symmetric_difference:
                std::for_each(other->keys().begin(), other->keys().end(), [&ret, this] (const Uuid& uid)
                {
                    if (this->keys().end() == this->keys().find(uid))
                    {
                        ret->insert(uid);
                    }
                });
                // fall through.
            case MergeMode::k_complement:
                std::for_each(this->keys().begin(), this->keys().end(), [&ret, other] (const Uuid& uid)
                {
                    if (other->keys().end() == other->keys().find(uid))
                    {
                        ret->insert(uid);
                    }
                });
                break;
        }
        return std::unique_ptr<Index>(ret);
    }

    void Index::place(const void* const key,
                      const size_t key_len,
                      const lj::Uuid& uid)
    {
        size_t sz;
        const uint8_t* const pk = uid.data(&sz);
        record(key, key_len, pk, sz);
    }

    void Index::place(const lj::Bson& item)
    {
        const lj::Uuid& uid = lj::bson_as_uuid(item["__uid"]);
        size_t sz;
        const uint8_t* const pk = uid.data(&sz);
        char* data = item.to_binary();
        record(pk, sz, data, item.size());
        delete[] data;
    }

    void Index::remove(const void* const key,
                       const size_t key_len,
                       const lj::Uuid& uid)
    {
        size_t sz;
        const uint8_t* const pk = uid.data(&sz);
        erase(key, key_len, pk, sz);
    }

    void Index::remove(const lj::Bson& item)
    {
        const lj::Uuid& uid = lj::bson_as_uuid(item["__uid"]);
        size_t sz;
        const uint8_t* const pk = uid.data(&sz);
        char* data = item.to_binary();
        erase(pk, sz, data, item.size());
        delete[] data;
    }

    void Index::check(const void* const key,
                      const size_t key_len,
                      const lj::Uuid& uid)
    {
        size_t sz;
        const uint8_t* const pk = uid.data(&sz);
        test(key, key_len, pk, sz);
    }

    void Index::check(const lj::Bson& item)
    {
        const lj::Uuid& uid = lj::bson_as_uuid(item["__uid"]);
        size_t sz;
        const uint8_t* const pk = uid.data(&sz);
        char* data = item.to_binary();
        test(pk, sz, data, item.size());
        delete[] data;
    }

    bool Index::items(std::list<Bson>& records) const
    {
        return storage()->vault()->fetch(this, records);
    }
    
    bool Index::items(std::list<Bson*>& records) const
    {
        return storage()->vault()->fetch(this, records);
    }

    bool Index::items_raw(lj::Bson& records) const
    {
        return storage()->vault()->fetch_raw(this, records);
    }
    
    bool Index::first(lj::Bson& result) const
    {
        return storage()->vault()->fetch_first(this, result);
    }
        
    const lj::Storage* const Index::storage() const
    {
        return storage_;
    }

    Vault::Vault(const lj::Storage* const storage) : lj::Index(storage)
    {
    }

    Vault::Vault(const lj::Vault* const vault) : lj::Index(vault)
    {
    }

    Vault::~Vault()
    {
    }
}; // namespace lj.
