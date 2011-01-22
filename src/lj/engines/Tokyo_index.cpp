/*!
 \file lj/engines/Tokyo_index.cpp
 \brief LJ Tokyo Index engine implementation.
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

#include "lj/engines/Tokyo_index.h"

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

    void tune_tree_db(TCBDB* db, const void* ptr)
    {
        const lj::Bson* bn = static_cast<const lj::Bson*>(ptr);
        if (lj::bson_as_string(bn->nav("compare")).compare("lex") == 0)
        {
            tcbdbsetcmpfunc(db, tcbdbcmplexical, NULL);
            lj::Log::info.log("  Using lexical for compares") << lj::Log::end;
        }
        else if (lj::bson_as_string(bn->nav("compare")).compare("int32") == 0)
        {
            tcbdbsetcmpfunc(db, tcbdbcmpint32, NULL);
            lj::Log::info.log("  Using int32 for compares") << lj::Log::end;
        }
        else
        {
            tcbdbsetcmpfunc(db, tcbdbcmpint64, NULL);
            lj::Log::info.log("  Using int64 for compares") << lj::Log::end;
        }
        tcbdbtune(db, 256, 512, 65498, 9, 11, BDBTLARGE | BDBTBZIP);
    }
}; // namespace (anonymous)


namespace lj
{
    namespace engines
    {
        Tokyo_index::Tokyo_index(const lj::Bson* const server_config,
                                 const lj::Bson* const storage_config,
                                 const lj::Bson* const index_config,
                                 const lj::Storage* const storage) :
                                 lj::Index(storage),
                                 is_unique_constraint_(false),
                                 server_config_(server_config),
                                 storage_config_(storage_config),
                                 index_config_(index_config),
                                 keys_()
        {
            is_unique_constraint_ = lj::bson_as_boolean(index_config_->nav("constraint/unique"));

            if (is_unique_constraint)
            {
                hash_ = open_db<tokyo::Hash_db>(*server_config_,
                                                *storage_config_,
                                                *index_config_,
                                                "hash",
                                                k_hash_db_mode,
                                                &tune_hash_db)
            }

            tree_ = open_db<toky::Tree_db>(*server_config_,
                                           *storage_config_,
                                           *index_config,
                                           "tree",
                                           k_tree_db_mode,
                                           &tune_tree_db);

        }

        Tokyo_index::Tokyo_index(const Tokyo_index* const orig) :
                                 lj::Index(orig->storage_),
                                 is_unique_constraint(orig->is_unique_constraint_),
                                 tree_(orig->tree_),
                                 hash_(orig->hash_),
                                 server_config_(orig->server_config_),
                                 storage_config_(orig->storage_config_),
                                 index_config_(orig->index_config_),
                                 keys_()
        {
        }

        Tokyo_index::~Tokyo_index()
        {
            tree_.reset();
            hash_.reset();
        }

        std::unique_ptr<Index> Tokyo_index::equal(const void* const val,
                                                  const size_t len) const
        {
            Tokyo_index* ret = this->clone();
            if (is_unique_constraint)
            {
                auto pair = hash_->at(val, len);
                uint8_t* bytes = static_cast<uint8_t*>(pair.first);
                if (bytes && 16 == pair.second)
                {
                    ret->include(Uuid(bytes));
                }
                free(bytes);
            }
            else
            {
                tokyo::DB::list_value_t pairs;
                tree_->at_together(val, len, pairs);
                for (auto pair : pairs)
                {
                    uint8_t* bytes = static_cast<uint8_t*>(pair.first);
                    if (bytes && 16 == pair.second)
                    {
                        ret->insert(Uuid(bytes));
                    }
                    free(bytes);
                }
            }
            return ret;
        }
        
        std::unique_ptr<Index> Tokyo_index::greater(const void* const val,
                                                    const size_t len) const
        {
            Tokyo_index* ret = this->clone();
            auto max = tree_->max_key();
            if (max.first)
            {
                tokyo::DB::list_value_t pairs;
                tree_->at_range(val,
                                len,
                                false,
                                max.first,
                                max.second,
                                true,
                                pairs);
                for (auto pair : pairs)
                {
                    uint8_t* bytes = static_cast<uint8_t*>(pair.first);
                    if (bytes && 16 == pair.second)
                    {
                        ret->insert(Uuid(bytes));
                    }
                    free(bytes);
                }
            }
            return ret;
        }
        
        std::unique_ptr<Index> Tokyo_index::lesser(const void* const val,
                                                   const size_t len) const
        {
            Tokyo_index* ret = this->clone();
            auto min = tree_->min_key();
            if (max.first)
            {
                tokyo::DB::list_value_t pairs;
                tree_->at_range(min.first,
                                min.second,
                                true,
                                val,
                                len,
                                false,
                                pairs);
                for (auto pair : pairs)
                {
                    uint8_t* bytes = static_cast<uint8_t*>(pair.first);
                    if (bytes && 16 == pair.second)
                    {
                        ret->insert(Uuid(bytes));
                    }
                    free(bytes);
                }
            }
            return ret;
        }

        void place(const void* const key,
                   const size_t key_len,
                   const void* const val,
                   const size_t val_len);

        void remove(const void* const key,
                    const size_t key_len,
                    const void* const val,
                    const size_t val_len);
    }; // namespace lj::engines
}; // namespace lj.
