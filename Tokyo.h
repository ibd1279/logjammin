#pragma once
/*
 \file Tokyo.h
 \author Jason Watson
 Copyright (c) 2009, Jason Watson
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

extern "C" {
#include <tcutil.h>
#include <tcbdb.h>
#include <dystopia.h>
#include <laputa.h>
}
#include <set>
#include <string>
#include <list>
#include "Exception.h"

namespace tokyo {

    //! Abstract DB Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 9, 2010
     */
    class DB {
    protected:
        DB();
    public:
        
        //! Type for returned database values.
        typedef std::pair<void *, size_t> value_t;
        
        //! Type for returned database lists.
        typedef std::list<value_t> list_value_t;
        
        //! Destructor.
        virtual ~DB();
        
        //! Read all values together at a key.
        /*!
         \par
         The pointer returned is allocated with "malloc" and must be
         released with "free".
         \exception Exception Thrown on any error performing the action.
         */        
        virtual value_t at(const void *key,
                           const size_t len) = 0;
        
        //! Read all values together at a key.
        /*!
         \par
         The pointers returned are allocated with "malloc" and must be
         released with "free".
         \exception Exception Thrown on any error performing the action.
         */
        virtual bool at_together(const void *key,
                                 const size_t len,
                                 list_value_t &results) = 0;

        //! Place a value at a key.
        virtual void place(const void *key,
                           const size_t key_len,
                           const void * const val,
                           const size_t val_len) = 0;
        
        //! Place a value after any existing records at a specific key.
        virtual void place_with_existing(const void *key,
                                         const size_t key_len,
                                         const void * const val,
                                         const size_t val_len) = 0;
        
        //! Place a list of values together at a specific key.
        virtual void place_together(const void *key,
                                    const size_t key_len,
                                    const list_value_t &vals) = 0;
        
        //! Place a value at a specific key only if no record exists.
        virtual void place_if_absent(const void *key, 
                                     const size_t key_len, 
                                     const void * const val, 
                                     const size_t val_len) = 0;
        
        //! Place or append a value at a specific key.
        virtual void place_or_append(const void *key, 
                                     const size_t key_len, 
                                     const void * const val,
                                     const size_t val_len) = 0;
        
        //! Remove the first record matching at a specific key.
        virtual void remove(const void *key,
                            const size_t len) = 0;
        
        //! Remove all records together at a specific key.
        virtual void remove_together(const void *key,
                                     const size_t len) = 0;
        
        //! Remove the provided record from the existing records.
        virtual void remove_from_existing(const void *key,
                                          const size_t len,
                                          const void * const val, 
                                          const size_t val_len) = 0;
        
        //! Start the transaction.
        virtual void start_writes() = 0;
        
        //! Write the transaction.
        virtual void save_writes() = 0;
        
        //! Rollback the transaction.
        virtual void abort_writes() = 0;
    };
    
    //! Abstract Searcher Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class Searcher {
    protected:
        Searcher() {
        }
    public:
        //! type for the indexed search values.
        typedef std::string value_t;
        
        //! type for the document ID associated with the search value.
        typedef unsigned long long key_t;
        
        //! type for a collection of IDs.
        typedef std::set<unsigned long long> set_key_t;
        
        virtual ~Searcher() { }
        
        //! Index a document.
        virtual void index(const key_t key,
                           const value_t &txt) = 0;
        
        //! Remove a document.
        virtual void remove(const key_t key,
                            const value_t &txt) = 0;
        
        //! Search for a document.
        virtual bool search(const std::string &query,
                            set_key_t &results) = 0;
    };

    //! Tree DB Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 9, 2010
     */
    class TreeDB : public DB {
    private:
        //! The Tree datastructure.
        TCBDB *_db;
        
    protected:
        //! Get the underlying Database object.
        /*!
         \return The pointer to the database object.
         */
        inline TCBDB *db() {
            return _db;
        }
        
    public:
        //! Tree DB Constructor.
        TreeDB(const std::string &filename,
               const int mode,
               void (*db_tune_func)(TCBDB *));
        
        //! Tree DB Distructor.
        virtual ~TreeDB();
        
        virtual value_t at(const void *key,
                           const size_t len);
        virtual bool at_together(const void *key,
                                 const size_t len,
                                 list_value_t &results);
        virtual void place(const void *key,
                           const size_t key_len,
                           const void * const val, 
                           const size_t val_len);        
        virtual void place_with_existing(const void *key,
                                         const size_t key_len,
                                         const void * const val,
                                         const size_t val_len);
        virtual void place_together(const void *key,
                                    const size_t key_len,
                                    const list_value_t &vals);
        virtual void place_if_absent(const void *key, 
                                     const size_t key_len, 
                                     const void * const val,
                                     const size_t val_len);
        virtual void place_or_append(const void *key, 
                                     const size_t key_len, 
                                     const void * const val,
                                     const size_t val_len);
        virtual void remove(const void *key,
                            const size_t len);        
        virtual void remove_together(const void *key,
                                     const size_t len);
        virtual void remove_from_existing(const void *key,
                                          const size_t len,
                                          const void * const val,
                                          const size_t val_len);
        virtual void start_writes();
        virtual void save_writes();
        virtual void abort_writes();
        
        //! Get all of the values between a start and end key.
        virtual bool at_range(const void *start,
                              const size_t start_len,
                              const bool start_inc,
                              const void *end,
                              const size_t end_len,
                              const bool end_inc,
                              list_value_t &results);
                
        //! Get the maximum key value in the DB.
        virtual value_t max_key();
        
        //! Get the minimum key value in the DB.
        virtual value_t min_key();
        
        //! Get the keys in the range between start and end.
        virtual bool range_keys(const void *start,
                                const size_t start_len,
                                bool start_inc,
                                const void *end,
                                const size_t end_len,
                                bool end_inc,
                                list_value_t &keys);
    };
        
    //! Full text searcher.
    /*!
     Wraps a Tokyo Dystopia full text index object.
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class TextSearcher : public Searcher {
    private:
        //! The Full text database object.
        TCQDB *_db;
    protected:
        //! Get the underlying searcher object.
        inline TCQDB *db() {
            return _db;
        }
        
    public:
        //! Create a new searcher.
        TextSearcher(const std::string &filename,
                     const int mode,
                     void (*db_tune_func)(TCQDB *));
        virtual ~TextSearcher();
        virtual void index(const key_t key,
                           const value_t &txt);
        virtual void remove(const key_t key,
                            const value_t &txt);
        virtual bool search(const std::string &query,
                            set_key_t &results);
        //! optimize the searcher db.
        void optimize();
        
        //! empty the searcher db.
        void truncate();
    };
    
    //! Tag searcher.
    /*!
     Wraps a Tokyo Dystopia word index object.
     \author Jason Watson
     \version 1.0
     \date April 19, 2010
     */
    class TagSearcher : public Searcher {
    private:
        //! The word database object.
        TCWDB *_db;
    protected:
        //! Get the underlying searcher object.
        inline TCWDB *db() {
            return _db;
        }
        
    public:
        //! Type of lists of values.
        typedef std::set<value_t> set_value_t;
        
        //! Crete a new Tag Searcher.
        TagSearcher(const std::string &filename,
                 const int mode,
                 void (*db_tune_func)(TCWDB *));
        virtual ~TagSearcher();
        virtual void index(const key_t key,
                           const value_t &txt);
        virtual void remove(const key_t key,
                            const value_t &txt);
        virtual bool search(const std::string &query,
                            set_key_t &results);

        //! index a key to multiple words/phrases.
        virtual void index(const key_t key,
                           const set_value_t &words);
        //! remove a key from multiple words/phrases.
        virtual void remove(const key_t key,
                            const set_value_t &words);

        //! optimize the server db.
        void optimize();
        
        //! empty the searcher db.
        void truncate();
    };
};