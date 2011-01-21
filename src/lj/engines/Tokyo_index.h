#pragma once
/*!
 \file lj/engines/Tokyo_index.h
 \brief LJ Tokyo Index engine interface.
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
        //! The Tokyo Cabinet Index engine.
        /*!
         \author Jason Watson
         \version 1.0
         \date January 18, 2011
         \sa lj::Storage
         */
        class Tokyo_index : public lj::Index {
        public:
            //! Constructor.
            Tokyo_index(const lj::Bson* const server_config,
                        const lj::Bson* const storage_config,
                        const lj::Bson* const index_config,
                        const lj::Storage* const storage);
            
            //! Destructor.
            virtual ~TokyoIndex();

            virtual std::unique_ptr<Index> equal(const void* const val,
                                                 const size_t len) const;
            
            virtual std::unique_ptr<Index> greater(const void* const val,
                                                   const size_t len) const;
            
            virtual std::unique_ptr<Index> lesser(const void* const val,
                                                  const size_t len) const;

            virtual std::unique_ptr<Index> merge(const lj::Index::MergeMode mode,
                                                  const Index* const other);

            virtual void place(const void* const key,
                               const size_t key_len,
                               const void* const val,
                               const size_t val_len);

            virtual void remove(const void* const key,
                                const size_t key_len,
                                const void* const val,
                                const size_t val_len);

            virtual uint64_t size() const
            {
                return keys_.size();
            }

            virtual const std::list<lj::Uuid>& keys() const
            {
                return keys_;
            }

        private:
            std::shared_ptr<tokyo::Tree_db> tree_;
            std::shared_ptr<tokyo::Hash_db> hash_;
            std::shared_ptr<lj::Bson> config_;
            std::list<lj::Uuid> keys_;
        };
    }; // namespace lj::engines
}; // namespace lj.
