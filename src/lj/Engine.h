#pragma once
/*!
 \file lj/Engine.h
 \brief LJ Storage engine interface.
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

#include "lj/Bson.h"
#include "lj/Storage.h"

#include <memory>
#include <string>

namespace lj
{
    class Storage;
    
    //! A key/value storage interface for different storage indices.
    /*!
     \author Jason Watson
     \version 1.0
     \date January 18, 2011
     \sa lj::Storage
     */
    class Index {
    public:
        //! Set operation to use or merging two results.
        enum class MergeMode
        {
            k_intersection,        //!< Similar to an "AND".
            k_union,               //!< Similar to an "OR".
            k_complement,          //!< Similar to a "NOT".
            k_symmetric_difference //!< Similar to a "XOR"
        };
        
        //! Constructor.
        Index(const lj::Storage* const storage) : storage_(storage)
        {
        }
        
        //! Destructor.
        virtual ~Index()
        {
        }

        virtual std::unique_ptr<Index> equal(const void* const val,
                                             const size_t len) const = 0;
        
        virtual std::unique_ptr<Index> eq(const std::string& indx,
                                          const void* const val,
                                          const size_t len,
                                          const MergeMode mode = MergeMode::k_intersection) const
        {
            return storage()->index(indx)->equal(val, len)->merge(mode, this);
        }
        
        virtual std::unique_ptr<Index> greater(const void* const val,
                                          const size_t len) const = 0;

        virtual std::unique_ptr<Index> gt(const std::string& indx,
                                          const void* const val,
                                          const size_t len,
                                          const MergeMode mode = MergeMode::k_intersection) const
        {
            return storage()->index(indx)->greater(val, len)->merge(mode, this);
        }
        
        virtual std::unique_ptr<Index> lesser(const void* const val,
                                           const size_t len) const = 0;

        virtual std::unique_ptr<Index> lt(const std::string& indx,
                                           const void* const val,
                                           const size_t len,
                                           const MergeMode mode = MergeMode::k_intersection) const
        {
            return storage()->index(indx)->lesser(val, len)->merge(mode, this);
        }

        virtual std::unique_ptr<Index> merge(const MergeMode mode,
                                             const Index* const other) = 0;
        
        virtual void place(const void* const key,
                           const size_t key_len,
                           const void* const val,
                           const size_t val_len) = 0;

        virtual void remove(const void* const key,
                            const size_t key_len,
                            const void* const val,
                            const size_t val_len) = 0;

        virtual uint64_t size() const = 0;

        virtual const std::list<lj::Uuid>& keys() const = 0;

        virtual bool items(std::list<Bson>& records) const
        {
            return storage()->vault()->items(this, records);
        }
        
        virtual bool items(std::list<Bson*>& records) const
        {
            return storage()->vault()->items(this, records);
        }

        virtual bool items_raw(lj::Bson& records) const
        {
            return storage()->vault()->items_raw(this, records);
        }
        
        virtual bool first(lj::Bson& result) const
        {
            return storage()->vault()->first(this, result);
        }
        
    protected:
        virtual const lj::Storage* const storage() const
        {
            return storage_;
        }

    private:
        const Storage* const storage_;
    }; // class lj::Index.

    //! A key/value storage interface for different storage vaults.
    /*!
     \author Jason Watson
     \version 1.0
     \date January 19, 2011
     \sa lj::Storage
     */
    class Vault
    {
    public:
        Vault()
        {
        }

        virtual ~Vault()
        {
        }

        virtual uint64_t next_key() = 0;

        virtual void journal_begin(const lj::Uuid& uid) = 0;

        virtual void place(lj::Bson& item) = 0;

        virtual void remove(lj::Bson& item) = 0;

        virtual void journal_end(const lj::Uuid& uid) = 0;

        virtual uint64_t size() = 0;

        virtual bool items(const lj::Index* const index,
                           std::list<Bson>& records) const = 0;
        
        virtual bool items(const lj::Index* const index,
                           std::list<Bson*>& records) const = 0;

        virtual bool items_raw(const lj::Index* const index,
                               lj::Bson& records) const = 0;
        
        virtual bool first(const lj::Index* const index,
                           lj::Bson& result) const = 0;
    }; // class lj::Vault
}; // namespace lj.
