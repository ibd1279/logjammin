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

#include <cassert>

namespace
{
    const int k_tree_db_mode = BDBOREADER | BDBOWRITER | BDBOCREAT | BDBOLCKNB;
    const int k_hash_db_mode = HDBOREADER | HDBOWRITER | HDBOCREAT | HDBOLCKNB;
    const int k_fixed_db_mode = FDBOREADER | FDBOWRITER | FDBOCREAT | FDBOLCKNB;

    template<typename T>
    T* open_db(const lj::Bson& server_config,
               const lj::Bson& storage_config,
               const lj::Bson& vault_config,
               const std::string& path,
               const int open_flags,
               typename T::Tune_function_pointer tune_function)
    {
        const lj::Bson* cfg = vault_config[path];
        std::string& r = lj::bson_as_string(server_config["server/directory"]);
        std::string& s = lj::bson_as_string(storage_config["storage/name"]);
        std::string& p = lj::bson_as_string(cfg->nav("filename"));
        assert(p.size() > 0);
        std::string filename(p.at(0) == '/' ? p : (r + "/" + s + "/" + p));
        return new T(filename, open_flags, tune_function, cfg);
    }

    void tune_hash_db(TCHDB* db, const void* ptr)
    {
        //const Bson *bn = static_cast<const Bson *>(ptr);
        tchdbtune(db, 1000003, 8, 11, HDBTLARGE | HDBTBZIP);
    }

    void tune_key_db(TCBDB* db, const void* ptr)
    {
        const lj::Bson* bn = static_cast<const lj::Bson*>(ptr);
        tcbdbsetcmpfunc(db, tcbdbcmpint64, NULL);
        tcbdbtune(db, 256, 512, 65498, 9, 11, BDBTLARGE | BDBTBZIP);
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
                                 const lj::Bson* const vault_config) :
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
            key_ = open_db<tokyo::Tree_db>(*server_config_,
                                           *storage_config_,
                                           *vault_config_,
                                           "key",
                                           k_tree_db_mode,
                                           tune_key_db);
            journal = open_db<tokyo::Fixed_db>(*server_config_,
                                               *storage_config_,
                                               *vault_config_,
                                               "journal",
                                               k_fixed_db_mode,
                                               tune_journal_db);
        }
        
        Tokyo_vault::~Tokyo_vault()
        {
            data_.reset();
            key_.reset();
            journal.reset();
        }

        uint64_t Tokyo_vault::next_key()
        {
            uint64_t *ptr = static_cast<uint64_t*>(key_->max_key().first);
            return (*ptr) + 1;
        }

        void Tokyo_vault::place(lj::Bson& item)
        {
            try
            {
                data_->start_writes();
                key_->start_writes();

                lj::Uuid& uid = lj::bson_as_uuid(item["__uid"]);
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                data_->place(pk
                             sz,
                             item.to_binary(),
                             item.size());

                uint64_t key = lj::bson_as_uint64(item["__key"]);
                key_->place(key,
                            sizeof(uint64_t),
                            pk,
                            sz);

                key_->commit_writes();
                data_->commit_writes();
            }
            catch (lj::Exception* ex)
            {
                key_->abort_writes();
                data_->abort_writes();
                throw ex;
            }
        }

        void Tokyo_vault::remove(lj::Bson& item)
        {
            try
            {
                data_->start_writes();
                key_->start_writes();

                lj::Uuid& uid = lj::bson_as_uuid(item["__uid"]);
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                uint64_t key = lj::bson_as_uint64(item["__key"]);
                key_->remove_from_existing(key,
                                           sizeof(uint64_t),
                                           pk,
                                           sz);

                data_->remove(&pk,
                              sz);

                key_->commit_writes();
                data_->commit_writes();
            }
            catch (lj::Exception* ex)
            {
                key_->abort_writes();
                data_->abort_writes();
                throw ex;
            }
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
                journal_->commit_writes();
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
                journal_->commit_writes();
            }
            catch (lj::Exception* ex)
            {
                journal_->abort_writes();
                throw ex;
            }
        }

        uint64_t Tokyo_vault::size()
        {
            return data_->count();
        }

        bool Tokyo_vault::items(const lj::Index* const index,
                                std::list<Bson>& records) const
            
        {
            for (lj::Uuid& uid : index.keys())
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                char* item = static_cast<char*>(data_->at(pk, sz).first);
                if (!item)
                {
                    continue;
                }
                record.push_back(lj::Bson(Bson::k_document, item));
                free(item);
            }
            return records.size();;
        }

        bool Tokyo_vault::items(const lj::Index* const index,
                                std::list<Bson*>& records) const
        {
            for (lj::Uuid& uid : index.keys())
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                char* item = static_cast<char*>(data_->at(pk, sz).first);
                if (!item)
                {
                    continue;
                }
                record.push_back(new lj::Bson(Bson::k_document, item));
                free(item);
            }
            return records.size();
        }

        bool Tokyo_vault::items_raw(const lj::Index* const index,
                                    lj::Bson& records) const
        {
            for (lj::Uuid& uid : index.keys())
            {
                size_t sz;
                const uint8_t* const pk = uid.data(&sz);
                char* item = static_cast<char*>(data_->at(pk, sz).first);
                if (!item)
                {
                    continue;
                }
                record.push_child("", new lj::Bson(Bson::k_binary_document,
                                                   item));
                free(item);
            }
            return records.size();
        }

        bool Tokyo_vault::first(const lj::Index* const index,
                                lj::Bson& result) const
        {
            for (lj::Uuid& uid : index.keys())
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
