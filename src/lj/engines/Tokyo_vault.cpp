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

#include "lj/engines/Tokyo_vault.h"

#include <cassert>

namespace
{
    const int k_tree_db_mode = BDBOREADER | BDBOWRITER | BDBOCREAT | BDBOLCKNB;
    const int k_hash_db_mode = HDBOREADER | HDBOWRITER | HDBOCREAT | HDBOLCKNB;
    const int k_fixed_db_mode = FDBOREADER | FDBOWRITER | FDBOCREAT | FDBOLCKNB;

    template<typename T>
    std::shared_ptr<T> open_db(const lj::Bson& server_config,
                               const lj::Bson& storage_config,
                               const lj::Bson& vault_config,
                               const std::string& path,
                               const int open_flags,
                               typename T::Tune_function_pointer tune_function)
    {
        const lj::Bson* cfg = vault_config.path(path);
        const std::string& r = lj::bson_as_string(server_config["server/directory"]);
        const std::string& s = lj::bson_as_string(storage_config["storage/name"]);
        const std::string& p = lj::bson_as_string(cfg->nav("filename"));
        assert(p.size() > 0);
        std::string filename(p.at(0) == '/' ? p : (r + "/" + s + "/" + p));
        return std::shared_ptr<T>(new T(filename, open_flags, tune_function, cfg));
    }

    void tune_hash_db(TCHDB* db, const void* ptr)
    {
        //const Bson *bn = static_cast<const Bson *>(ptr);
        tchdbtune(db, 1000003, 8, 11, HDBTLARGE | HDBTBZIP);
    }

    void tune_journal_db(TCFDB* db, const void* ptr)
    {
        tcfdbtune(db, 16, -1);
    }

}; // namespace (anonymous)

namespace lj
{
    namespace engines
    {
        Tokyo_vault::Tokyo_vault(const lj::Bson* const server_config,
                                 const lj::Bson* const storage_config,
                                 const lj::Bson* const vault_config,
                                 const lj::Storage* const storage) :
                                 lj::Vault(storage),
                                 server_config_(server_config),
                                 storage_config_(storage_config),
                                 vault_config_(vault_config)
        {
            data_ = open_db<tokyo::Hash_db>(*server_config_,
                                            *storage_config_,
                                            *vault_config_,
                                            "data",
                                            k_hash_db_mode,
                                            tune_hash_db);
            journal_ = open_db<tokyo::Fixed_db>(*server_config_,
                                                *storage_config_,
                                                *vault_config_,
                                                "journal",
                                                k_fixed_db_mode,
                                                tune_journal_db);
        }

        Tokyo_vault::Tokyo_vault(const lj::engines::Tokyo_vault* const orig) :
                                 lj::Vault(orig),
                                 data_(orig->data_),
                                 journal_(orig->journal_),
                                 server_config_(orig->server_config_),
                                 storage_config_(orig->storage_config_),
                                 vault_config_(orig->vault_config_)
        {
        }
        
        Tokyo_vault::~Tokyo_vault()
        {
            data_.reset();
            journal_.reset();
        }

        lj::engines::Tokyo_vault* Tokyo_vault::clone() const
        {
            return new lj::engines::Tokyo_vault(this);
        }

        std::unique_ptr<Index> Tokyo_vault::equal(const void* const val,
                                                  const size_t len) const
        {
            lj::engines::Tokyo_vault* ret = this->clone();
            auto pair = data_->at(val, len);
            uint8_t* bytes = static_cast<uint8_t*>(pair.first);
            if (bytes)
            {
                ret->insert(Uuid(static_cast<const uint8_t* const>(val)));
            }
            free(bytes);
            return std::unique_ptr<Index>(ret);
        }
        
        std::unique_ptr<Index> Tokyo_vault::greater(const void* const val,
                                                    const size_t len) const
        {
            throw new lj::Exception("Tokyo_vault",
                                    "Unsupported operation [greaters] on vault.");
        }
        
        std::unique_ptr<Index> Tokyo_vault::lesser(const void* const val,
                                                   const size_t len) const
        {
            throw new lj::Exception("Tokyo_vault",
                                    "Unsupported operation [lesser] on vault.");
        }

        void Tokyo_vault::record(const void* const key,
                                 const size_t key_len,
                                 const void* const val,
                                 const size_t val_len)
        {
            try
            {
                data_->start_writes();

                data_->place(key,
                             key_len,
                             val,
                             val_len);

                data_->save_writes();
            }
            catch (lj::Exception* ex)
            {
                data_->abort_writes();
                throw ex;
            }
        }

        void Tokyo_vault::erase(const void* const key,
                                const size_t key_len,
                                const void* const val,
                                const size_t val_len)
        {
            try
            {
                data_->start_writes();

                data_->remove(key,
                              key_len);

                data_->save_writes();
            }
            catch (lj::Exception* ex)
            {
                data_->abort_writes();
                throw ex;
            }
        }

        void Tokyo_vault::test(const void* const key,
                               const size_t key_len,
                               const void* const val,
                               const size_t val_len) const
        {
            auto pair = data_->at(key, key_len);
            uint8_t* bytes = static_cast<uint8_t*>(pair.first);
            if (!bytes)
            {
                return;
            }

            if (val_len != pair.second)
            {
                free(bytes);
                throw new lj::Exception("Tokyo_vault",
                                        "Unique constraint violation.");
            }

            for (size_t h = 0; h < val_len; ++h)
            {
                if (static_cast<const uint8_t* const>(val)[h] != bytes[h])
                {
                    free(bytes);
                    throw new lj::Exception("Tokyo_vault",
                                            "Unique constraint violation.");
                }
            }
            free(bytes);
        }

        void Tokyo_vault::journal_begin(const lj::Uuid& uid)
        {
            try
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                const uint64_t key = static_cast<uint64_t>(uid);
                journal_->start_writes();
                journal_->place(&key,
                                sizeof(uint64_t),
                                pk,
                                sz);
                journal_->save_writes();
            }
            catch (lj::Exception* ex)
            {
                journal_->abort_writes();
                throw ex;
            }
        }

        void Tokyo_vault::journal_end(const lj::Uuid& uid)
        {
            try
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                const uint64_t key = static_cast<uint64_t>(uid);
                journal_->start_writes();
                journal_->place(&key,
                                sizeof(uint64_t),
                                pk,
                                sz);
                journal_->save_writes();
            }
            catch (lj::Exception* ex)
            {
                journal_->abort_writes();
                throw ex;
            }
        }

        uint64_t Tokyo_vault::count() const
        {
            return data_->count();
        }

        bool Tokyo_vault::fetch(const lj::Index* const index,
                                std::list<Bson>& records) const
            
        {
            for (const lj::Uuid& uid : index->keys())
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                char* item = static_cast<char*>(data_->at(pk, sz).first);
                if (!item)
                {
                    continue;
                }
                records.push_back(lj::Bson(Bson::k_document, item));
                free(item);
            }
            return records.size();;
        }

        bool Tokyo_vault::fetch(const lj::Index* const index,
                                std::list<Bson*>& records) const
        {
            for (const lj::Uuid& uid : index->keys())
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                char* item = static_cast<char*>(data_->at(pk, sz).first);
                if (!item)
                {
                    continue;
                }
                records.push_back(new lj::Bson(Bson::k_document, item));
                free(item);
            }
            return records.size();
        }

        bool Tokyo_vault::fetch_raw(const lj::Index* const index,
                                    lj::Bson& records) const
        {
            for (const lj::Uuid& uid : index->keys())
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                char* item = static_cast<char*>(data_->at(pk, sz).first);
                if (!item)
                {
                    continue;
                }
                records.push_child("", new lj::Bson(Bson::k_binary_document,
                                                   item));
                free(item);
            }
            return records.size();
        }

        bool Tokyo_vault::fetch_first(const lj::Index* const index,
                                      lj::Bson& result) const
        {
            for (const lj::Uuid& uid : index->keys())
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                char* item = static_cast<char*>(data_->at(pk, sz).first);
                if (!item)
                {
                    continue;
                }
                result.set_value(Bson::k_document, item);
                free(item);
                return true;
            }
            return false;
        }
    }; // namespace lj::engines
}; // namespace lj.
