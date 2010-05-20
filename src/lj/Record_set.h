#pragma once
/*!
 \file Record_set.h
 \brief LJ Record_set header.
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
#include "tokyo/Tokyo.h"

#include <list>
#include <memory>
#include <set>
#include <string>

namespace lj
{
    class Storage;
    
    //! Abstract collection of documents.
    /*!
     \par
     The Record_set is really an abstract representation of a set of
     documents. The refine, search, and tagged methods are used to modify
     the set of documents to match a certain set of criteria. The following
     examples shows a usage to get a list of all users with the first name
     "Jason" and the last name "Watson".
     \code
     // Example usage of lj::Storage and lj::Record_set.
     lj::Storage storage("user");
     std::list<lj::Bson> records;
     storage.equal("first_name", "Jason").equal("last_name", "Watson").items(records);
     \endcode
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     \sa lj::Storage
     */
    class Record_set {
    public:
        //! Set operation.
        enum Operation
        {
            k_intersection,        //!< Similar to an "AND".
            k_union,               //!< Similar to an "OR".
            k_complement,          //!< Similar to a "NOT".
            k_symmetric_difference //!< Similar to a "XOR"
        };
        
        //! Constructor.
        Record_set()
        {
        }
        
        //! Destructor.
        virtual ~Record_set()
        {
        }
        
        //! Set the Record_set operation.
        /*!
         \par
         The operation provided is used for creating the response object from
         the refine, search, and tagged methods.
         \param op The new default operation to perform.
         \return The modified Record_set object.
         */
        virtual Record_set& set_operation(const Record_set::Operation op) = 0;
        
        //! Check if the Record_set contains a key.
        /*!
         \param key The key to test for.
         \return True if the key is included. False otherwise.
         */
        virtual bool is_included(const unsigned long long key) const = 0;
        
        //! Add keys to the Record_set.
        /*!
         \par
         Keys are copied from the input set.
         \param keys The keys to add.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> include_keys(const std::set<unsigned long long>& keys) = 0;
        
        //! Add a key to the Record_set.
        /*!
         \param key The key to add.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> include_key(const unsigned long long key) = 0;
        
        //! Remove keys from the Record_set.
        /*!
         \param keys The keys to exclude.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> exclude_keys(const std::set<unsigned long long> &keys) = 0;
        
        //! Remove a key from the Record_set.
        /*!
         \param key The key to remove.
         \return The modified Record_set object.
         */
        virtual std::auto_ptr<Record_set> exclude_key(const unsigned long long key) = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for the value \c val. The results from the index are
         operated with the current Record_set.
         \par
         If a Hash_db exists for \c indx, it will be used.  If it does not exist,
         a Tree_db will be used. If neither index exists, a copy of the current
         Record_set is returned.
         \param indx The index to search against.
         \param val The value to search for.
         \param len The length of the value.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> equal(const std::string& indx,
                                                const void* const val,
                                                const size_t len) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values greater than \c val.
         The results from the index are operated with the current Record_set.
         \par
         If a Tree_db exists for \c indx, it will be used. If an index does
         not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param val The value to search for.
         \param len The length of the value.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> greater(const std::string& indx,
                                                  const void* const val,
                                                  const size_t len) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values lesser than \c val.
         The results from the index are operated with the current Record_set.
         \par
         If a Tree_db exists for \c indx, it will be used. If an index does
         not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param val The value to search for.
         \param len The length of the value.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> lesser(const std::string& indx,
                                                 const void* const val,
                                                 const size_t len) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values containing \c term.
         The results from the index are operated with the current Record_set.
         \par
         If a Text_searcher exists for \c indx, it will be used. If an index
         does not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param term The value to search for.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> contains(const std::string& indx,
                                                   const std::string& term) const = 0;
        
        //! Perform operation against this Record_set and another new Record_set.
        /*!
         \par
         Searches \c indx for documents with values containing \c word.
         The results from the index are operated with the current Record_set.
         \par
         If a Word_searcher exists for \c indx, it will be used. If an index
         does not exist, a copy of the current Record_set is returned.
         \param indx The index to search against.
         \param word The value to search for.
         \return A Record_set.
         */
        virtual std::auto_ptr<Record_set> tagged(const std::string& indx,
                                                 const std::string& word) const = 0;
        
        //! Record_set size.
        /*!
         \return The number of documents currently in the set.
         */
        virtual unsigned long long size() const = 0;
        
        //! Get the documents in this Record_set.
        /*!
         \par
         Get a list of items and return them in \c records.
         \param records The list to place items into.
         \return True when records have been added.
         */
        virtual bool items(std::list<Bson>& records) const = 0;
        
        //! Get the documents in this Record_set as pointers.
        /*!
         \par
         Get a list of items and return them in \c records.
         \param records The list to place items into.
         \return True when records have been added.
         */
        virtual bool items(std::list<Bson*>& records) const = 0;
        
        //! Get the first document in the Record_set.
        /*!
         \par
         Because the STL set backing the Record_set is ordered, the first
         document will always be the lowest key document in the set.
         \param record Record to populate.
         \return True when the record has been modified.
         */
        virtual bool first(Bson& result) const = 0;
    protected:
        static tokyo::Tree_db* storage_db(const Storage* s);
        static tokyo::Tree_db* storage_tree(const Storage* s, const std::string& indx);
        static tokyo::Hash_db* storage_hash(const Storage* s, const std::string& indx);
        static tokyo::TextSearcher* storage_text(const Storage* s, const std::string& indx);
        static tokyo::TagSearcher* storage_tag(const Storage* s, const std::string& indx);
    };
    
}; // namespace lj.