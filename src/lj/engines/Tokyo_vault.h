#pragma once
/*!
 \file lj/engines/Tokyo_vault.h
 \brief LJ Tokyo Vault Storage engine interface.
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
#include "tokyo/Tokyo.h"

namespace lj
{
    namespace engines
    {
        //! The Tokyo Cabinet storage engine.
        /*!
         \author Jason Watson
         \version 1.0
         \date January 18, 2011
         \sa lj::Storage
         */
        class Tokyo_vault : public lj::Vault {
        public:
            //! Constructor.
            Tokyo_vault(const lj::Bson* const server_config,
                        const lj::Bson* const storage_config,
                        const lj::Bson* const vault_config,
                        const lj::Storage* const storage);

            //! Copy-ish constructor.
            Tokyo_vault(const lj::engines::Tokyo_vault* const orig);
            
            //! Destructor.
            virtual ~Tokyo_vault();

            virtual lj::engines::Tokyo_vault* clone() const;

            virtual std::unique_ptr<Index> equal(const void* const val,
                                                 const size_t len) const;
            
            virtual std::unique_ptr<Index> greater(const void* const val,
                                                   const size_t len) const;
            
            virtual std::unique_ptr<Index> lesser(const void* const val,
                                                  const size_t len) const;

            virtual void record(const void* const key,
                                const size_t key_len,
                                const void* const val,
                                const size_t val_len);

            virtual void erase(const void* const key,
                               const size_t key_len,
                               const void* const val,
                               const size_t val_len);

            virtual void test(const void* const key,
                              const size_t key_len,
                              const void* const val,
                              const size_t val_len) const;

            virtual void journal_begin(const lj::Uuid& uid);

            virtual void journal_end(const lj::Uuid& uid);

            virtual uint64_t count() const;

            virtual uint64_t size() const
            {
                return keys_.size();
            }

            virtual const std::set<lj::Uuid>& keys() const
            {
                return keys_;
            }

            virtual bool fetch(const lj::Index* const index,
                               std::list<Bson>& records) const;
            
            virtual bool fetch(const lj::Index* const index,
                               std::list<Bson*>& records) const;

            virtual bool fetch_raw(const lj::Index* const index,
                                   lj::Bson& records) const;
            
            virtual bool fetch_first(const lj::Index* const index,
                                     lj::Bson& result) const;
        protected:
            void insert(const lj::Uuid& uid)
            {
                keys_.insert(uid);
            }
            
        private:
            std::shared_ptr<tokyo::Hash_db> data_;
            std::shared_ptr<tokyo::Tree_db> key_;
            std::shared_ptr<tokyo::Fixed_db> journal_;
            const lj::Bson* const server_config_;
            const lj::Bson* const storage_config_;
            const lj::Bson* const vault_config_;
            std::set<lj::Uuid> keys_;
        };
    }; // namespace lj::engines
}; // namespace lj.
