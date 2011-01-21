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
                        const lj::Bson* const vault_config);
            
            //! Destructor.
            virtual ~Tokyo_vault();

            virtual void place(lj::Bson& item);

            virtual void remove(lj::Bson& item);

            virtual void journal_begin(const lj::Uuid& uid);

            virtual void journal_end(const lj::Uuid& uid);

            virtual uint64_t size();

            virtual bool items(const lj::Index* const index,
                               std::list<Bson>& records) const;
            
            virtual bool items(const lj::Index* const index,
                               std::list<Bson*>& records) const;

            virtual bool items_raw(const lj::Index* const index,
                                   lj::Bson& records) const;
            
            virtual bool first(const lj::Index* const index,
                               lj::Bson& result) const;
        private:
            std::shared_ptr<tokyo::Hash_db> data_;
            std::shared_ptr<tokyo::Tree_db> index_;
            std::shared_ptr<tokyo::Fixed_db> journal_;
            const lj::Bson* const server_config_;
            const lj::Bson* const storage_config_;
            const lj::Bson* const vault_config_;
        };
    }; // namespace lj::engines
}; // namespace lj.
