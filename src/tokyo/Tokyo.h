#pragma once
/*!
 \file Tokyo.h
 \brief Tokyo Cabinet / Tokyo Dystopia C++ wrapper
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

#include "lj/Exception.h"

extern "C" {
#include <tcutil.h>
#include <tcbdb.h>
#include <tchdb.h>
#include <tcfdb.h>
#include <dystopia.h>
#include <laputa.h>
}
#include <set>
#include <string>
#include <list>

namespace tokyo {
    
    //! Abstract DB Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 9, 2010
     */
    class DB {
    public:
        //! Type for returned database values.
        typedef std::pair<void*, size_t> value_t;
        
        //! Type for returned database lists.
        typedef std::list<value_t> list_value_t;
        
        //! Destructor.
        virtual ~DB();
        
        //! Read the value at a key.
        /*!
         \par
         The pointer returned is allocated with "malloc" and must be
         released with "free".
         \param key The record key.
         \param len The length of the record key.
         \return The value.
         \exception Exception Thrown on any error performing the action.
         */        
        virtual value_t at(const void* key,
                           const size_t len) = 0;
        
        //! Place a value at a key.
        virtual void place(const void* key,
                           const size_t key_len,
                           const void* const val,
                           const size_t val_len) = 0;
        
        //! Place a value at a specific key only if no record exists.
        virtual void place_if_absent(const void* key, 
                                     const size_t key_len, 
                                     const void* const val, 
                                     const size_t val_len) = 0;
        
        //! Place or append a value at a specific key.
        virtual void place_or_append(const void* key, 
                                     const size_t key_len, 
                                     const void* const val,
                                     const size_t val_len) = 0;
        
        //! Remove the first record matching at a specific key.
        virtual void remove(const void* key,
                            const size_t len) = 0;
        
        //! Start the transaction.
        virtual void start_writes() = 0;
        
        //! Write the transaction.
        virtual void save_writes() = 0;
        
        //! Rollback the transaction.
        virtual void abort_writes() = 0;
        
        //! Get the number of records in the database.
        virtual long long count() = 0;
        
        //! Delete all records from the database.
        virtual void truncate() = 0;
        
        //! Copy a file to a backup file.
        virtual void copy(const std::string &target) = 0;
    protected:
        DB();
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
                           const value_t& txt) = 0;
        
        //! Remove a document.
        virtual void remove(const key_t key,
                            const value_t& txt) = 0;
        
        //! Search for a document.
        virtual bool search(const std::string& query,
                            set_key_t& results) = 0;
        
        //! Erase the entire index.
        virtual void truncate() = 0;
    };
    
    //! Hash DB Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date May 9, 2010
     */
    class Hash_db : public DB {
    public:
        //! Type of the type function.
        typedef void (*Tune_function_pointer)(TCHDB*, const void*);
        
        //! Hash_db constructor
        /*!
         \par
         Create a new hash database object.
         \param filename The name of the file to open.
         \param mode The mode to open the file with.
         \param db_tune_func The function to use for tuning the database.
         \param ptr Additional argument to the \c db_tune_func.
         */
        Hash_db(const std::string& filename,
                const int mode,
                Tune_function_pointer db_tune_func,
                const void* ptr);
        
        //! Hash_db Destructor
        virtual ~Hash_db();
        
        virtual value_t at(const void* key,
                           const size_t len);
        virtual void place(const void* key,
                           const size_t key_len,
                           const void* const val, 
                           const size_t val_len);
        virtual void place_if_absent(const void* key, 
                                     const size_t key_len, 
                                     const void* const val,
                                     const size_t val_len);
        virtual void place_or_append(const void* key, 
                                     const size_t key_len, 
                                     const void* const val,
                                     const size_t val_len);
        virtual void remove(const void* key,
                            const size_t len);        
        virtual void start_writes();
        virtual void save_writes();
        virtual void abort_writes();
        virtual long long count();
        virtual void truncate();
        virtual void copy(const std::string &target);
        
    protected:
        //! Get the database handle.
        /*!
         \return The pointer to the database object.
         */
        inline TCHDB* db()
        {
            return db_;
        }
    private:
        //! Hiding to prevent use.
        /*!
         \param o Other.
         */
        Hash_db(const Hash_db& o);
        
        //! Hiding to prevent use.
        /*!
         \param o Other.
         */
        Hash_db& operator=(const Hash_db& o);
        
        //! Tokyo Hash Database handle.
        TCHDB *db_;
    };
    
    //! Tree DB Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date April 9, 2010
     */
    class Tree_db : public DB {
    public:
        //! Enumerator (Cursor).
        /*!
         \author Jason Watson
         \version 1.0
         \date May 20, 2010
         */
        class Enumerator {
        public:
            //! Enumerator direction
            enum Direction
            {
                k_forward,  //!< Enumerate the database from the beginning to the end.
                k_backward  //!< Enumerate the database from the end to the beginning.
            };
            
            //! Constructor
            Enumerator(TCBDB* db, Direction dir);
            
            //! Destructor.
            ~Enumerator();
            
            //! Test for more records.
            /*!
             When more() returns false, it deletes the object.
             */
            bool more();
            
            //! Advance the Enumerator.
            DB::value_t next();

            //! Get the next key.
            DB::value_t next_key();
        private:
            //! Hidden.
            Enumerator(const Enumerator& o);
            //! Hidden.
            Enumerator& operator=(const Enumerator& o);
            
            Direction dir_;
            BDBCUR* cur_;
            bool more_cache_;
        };
        
        //! Type of the type function.
        typedef void (*Tune_function_pointer)(TCBDB*, const void*);
        
        //! Tree DB Constructor.
        Tree_db(const std::string& filename,
                const int mode,
                Tune_function_pointer db_tune_func,
                const void* ptr);
        
        //! Tree DB Distructor.
        virtual ~Tree_db();
        
        virtual value_t at(const void* key,
                           const size_t len);
        
        //! Read all values together at a key.
        /*!
         \par
         The pointers returned are allocated with "malloc" and must be
         released with "free".
         \exception Exception Thrown on any error performing the action.
         */
        virtual bool at_together(const void* key,
                                 const size_t len,
                                 list_value_t& results);
        
        virtual void place(const void* key,
                           const size_t key_len,
                           const void* const val, 
                           const size_t val_len);
        
        //! Place a value after any existing records at a specific key.
        virtual void place_with_existing(const void* key,
                                         const size_t key_len,
                                         const void* const val,
                                         const size_t val_len);
        
        //! Place a list of values together at a specific key.
        virtual void place_together(const void* key,
                                    const size_t key_len,
                                    const list_value_t& vals);
        virtual void place_if_absent(const void* key, 
                                     const size_t key_len, 
                                     const void* const val,
                                     const size_t val_len);
        virtual void place_or_append(const void* key, 
                                     const size_t key_len, 
                                     const void* const val,
                                     const size_t val_len);
        
        virtual void remove(const void* key,
                            const size_t len);
        
        //! Remove all records together at a specific key.
        virtual void remove_together(const void* key,
                                     const size_t len);
        
        //! Remove the provided record from the existing records.
        virtual void remove_from_existing(const void* key,
                                          const size_t len,
                                          const void* const val,
                                          const size_t val_len);
        virtual void start_writes();
        virtual void save_writes();
        virtual void abort_writes();
        virtual long long count();
        virtual void truncate();
        virtual void copy(const std::string &target);
        
        //! Get all of the values between a start and end key.
        virtual bool at_range(const void* start,
                              const size_t start_len,
                              const bool start_inc,
                              const void* end,
                              const size_t end_len,
                              const bool end_inc,
                              list_value_t& results);
        
        //! Get the maximum key value in the DB.
        virtual value_t max_key();
        
        //! Get the minimum key value in the DB.
        virtual value_t min_key();
        
        //! Get the keys in the range between start and end.
        virtual bool range_keys(const void* start,
                                const size_t start_len,
                                bool start_inc,
                                const void* end,
                                const size_t end_len,
                                bool end_inc,
                                list_value_t& keys);
        
        //! Get an enumerator for touching every element.
        virtual Tree_db::Enumerator* forward_enumerator();
        
        //! Get an enumerator for touching every element.
        virtual Tree_db::Enumerator* backward_enumerator();
    protected:
        //! Get the database handle.
        /*!
         \return The pointer to the database object.
         */
        inline TCBDB* db() {
            return db_;
        }
    private:
        //! Hiding to prevent use.
        /*!
         \param o Other.
         */
        Tree_db(const Tree_db& o);
        
        //! Hiding to prevent use.
        /*!
         \param o Other.
         */
        Tree_db& operator=(const Tree_db& o);
        
        //! Tokyo Tree Database handle.
        TCBDB *db_;
    };
    
    //! Fixed DB Class.
    /*!
     \author Jason Watson
     \version 1.0
     \date May 9, 2010
     */
    class Fixed_db : public DB {
    public:
        //! Enumerator (Cursor).
        /*!
         \author Jason Watson
         \version 1.0
         \date May 20, 2010
         */
        class Enumerator {
        public:
            //! Enumerator direction
            enum Direction
            {
                k_forward,  //!< Enumerate the database from the beginning to the end.
                k_backward  //!< Enumerate the database from the end to the beginning.
            };
            
            //! Constructor
            Enumerator(TCFDB* db);
            
            //! Destructor.
            ~Enumerator();
            
            //! Test for more records.
            /*!
             When more() returns false, it deletes the object.
             */
            bool more();
            
            //! Advance the Enumerator.
            DB::value_t next();
            
            //! Get the next key.
            uint64_t next_key();
        private:
            //! Hidden.
            Enumerator(const Enumerator& o);
            //! Hidden.
            Enumerator& operator=(const Enumerator& o);
            
            uint64_t last_;
            bool more_cache_;
            TCFDB* db_;
        };
        
        //! Type of the type function.
        typedef void (*Tune_function_pointer)(TCFDB*, const void*);
        
        //! Fixed_db constructor
        /*!
         \par
         Create a new Fixed length record database object.
         \param filename The name of the file to open.
         \param mode The mode to open the file with.
         \param db_tune_func The function to use for tuning the database.
         \param ptr Additional argument to the \c db_tune_func.
         */
        Fixed_db(const std::string& filename,
                const int mode,
                Tune_function_pointer db_tune_func,
                const void* ptr);
        
        //! Fixed_db Destructor
        virtual ~Fixed_db();
        
        virtual value_t at(const void* key,
                           const size_t len);
        virtual void place(const void* key,
                           const size_t key_len,
                           const void* const val, 
                           const size_t val_len);
        virtual void place_if_absent(const void* key, 
                                     const size_t key_len, 
                                     const void* const val,
                                     const size_t val_len);
        virtual void place_or_append(const void* key, 
                                     const size_t key_len, 
                                     const void* const val,
                                     const size_t val_len);
        virtual void remove(const void* key,
                            const size_t len);        
        virtual void start_writes();
        virtual void save_writes();
        virtual void abort_writes();
        virtual long long count();
        virtual void truncate();
        virtual void copy(const std::string &target);
        
        //! Get an enumerator for touching every element.
        virtual Fixed_db::Enumerator* enumerator();
        
    protected:
        //! Get the database handle.
        /*!
         \return The pointer to the database object.
         */
        inline TCFDB* db()
        {
            return db_;
        }
    private:
        //! Hiding to prevent use.
        /*!
         \param o Other.
         */
        Fixed_db(const Fixed_db& o);
        
        //! Hiding to prevent use.
        /*!
         \param o Other.
         */
        Fixed_db& operator=(const Fixed_db& o);
        
        //! Tokyo Hash Database handle.
        TCFDB *db_;
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
        //! Type of the type function.
        typedef void (*Tune_function_pointer)(TCQDB*, const void*);
        
        //! Create a new searcher.
        TextSearcher(const std::string &filename,
                     const int mode,
                     Tune_function_pointer db_tune_func,
                     const void *ptr);
        virtual ~TextSearcher();
        virtual void index(const key_t key,
                           const value_t &txt);
        virtual void remove(const key_t key,
                            const value_t &txt);
        virtual bool search(const std::string &query,
                            set_key_t &results);
        virtual void truncate();
        
        //! optimize the searcher db.
        void optimize();
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
        //! Type of the type function.
        typedef void (*Tune_function_pointer)(TCWDB*, const void*);
        
        //! Type of lists of values.
        typedef std::set<value_t> set_value_t;
        
        //! Crete a new Tag Searcher.
        TagSearcher(const std::string &filename,
                    const int mode,
                    Tune_function_pointer db_tune_func,
                    const void *ptr);
        virtual ~TagSearcher();
        virtual void index(const key_t key,
                           const value_t &txt);
        virtual void remove(const key_t key,
                            const value_t &txt);
        virtual bool search(const std::string &query,
                            set_key_t &results);
        virtual void truncate();
        
        //! index a key to multiple words/phrases.
        virtual void index(const key_t key,
                           const set_value_t &words);
        
        //! remove a key from multiple words/phrases.
        virtual void remove(const key_t key,
                            const set_value_t &words);
        
        //! optimize the server db.
        void optimize();
        
    };
};