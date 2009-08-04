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
#include <iostream>

namespace tokyo {

    //! DB Exception type.
    /*!
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    struct Exception {
        
        //! Exception msg.
        std::string msg;
        
        //! Create a new exception object.
        /*!
         \param label The type of exception.
         \param emsg Exception message.
         */
        Exception(const char *label, const char *emsg) {
            msg = std::string(label).append(": ").append(emsg);
        }
    };

    //! DB Object to store full records.
    /*!
     Wraps a Tokyo Cabinet B+ Tree database. Provides the _put method to store to the
     database. Implementations should inherit from this class, and add members
     for the different indexes required.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<typename Key, typename Value>
    class DB {
    protected:
        //! The B+ tree database object.
        TCBDB *_db;
        
        //! Method for opening the database.
        void (*_open_func)(TCBDB *, int);
        
        //! Mode to open the database in.
        int _mode;
    public:
        //! Create and open the DB object.
        /*!
         \param db_open_func Method to use when opening the database.
         \param mode Mode flags to pass to the db open method.
         */
        DB(void (*db_open_func)(TCBDB *, int), int mode) {
            _open_func = db_open_func;
            _mode = mode;
            _db = tcbdbnew();
            (*_open_func)(_db, _mode);
        }
        
        //! Destructor
        virtual ~DB() {
            tcbdbclose(_db);
            tcbdbdel(_db);
        }
        
        //! Get the record associated with the primary key.
        /*!
         The value returned is allocated with malloc, and must be freed.
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key to get.
         \return The record associated with the primary key.
         */
        Value _at(const Key &key) {
            int sz = 0;
            Value *ptr = static_cast<Value *>(tcbdbget(_db, &key, sizeof(Key), &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }
                
        //! Store a record into the database.
        /*!
         If the record already exists, it is replaced.
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key for the record.
         \param value The record to store.
         \exception Exception When unable to put the record in the database.
         */
        void _put(const Key &key, const Value &value) {
            if(!tcbdbput(_db, &key, sizeof(Key), &value, sizeof(Value)))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
                
        //! Remove a record from the database.
        /*!
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key for the record.
         \exception Exception When unable to remove the record.
         */
        void _remove(const Key &key) {
            if(!tcbdbout(_db, &key, sizeof(Key)))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Get the maximum primary key value in the database.
        /*!
         \return The max primary key in the DB.
         \exception Exception When a database exception occurs.
         */
        Key max() {
            BDBCUR *cur = end();        
            
            int sz = 0;
            const Key *ptr = static_cast<const Key *>(tcbdbcurkey3(cur, &sz));
            tcbdbcurdel(cur);
            
            if(!ptr || !sz) {
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return *ptr;
        }
        
        //! Get the minimum primary key value in the database.
        /*!
         \return the min primary key in the DB.
         \exception Exception When a database exception occurs.
         */
        Key min() {
            BDBCUR *cur = begin();
            
            int sz = 0;
            const Key *ptr = static_cast<const Key *>(tcbdbcurkey3(cur, &sz));
            tcbdbcurdel(cur);
            
            if(!ptr || !sz) {
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return *ptr;
        }
        
        //! Return the number of records in the database.
        /*!
         \return Number of records.
         */
        inline unsigned long long size() {
            return tcbdbrnum(_db);
        }
        
        //! Get a cursor at a primary key.
        /*!
         \param key The primary key to place the cursor at.
         \return The cursor.
         \exception Exception If there was a database error.
         */
        BDBCUR *cursor(const Key &key) {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurjump(cur, &key, sizeof(Key))) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get a cursor at the first primary key.
        /*!
         \return The cursor at the first (min) primary key.
         \exception Exception If there was a database error.
         */
        BDBCUR *begin() {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurfirst(cur)) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get a cursor at the last primary key.
        /*!
         \return The cursor at the last (max) primary key.
         \exception Exception If there was a database error.
         */
        BDBCUR *end() {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurlast(cur)) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get the cursors key.
        /*!
         \param cur The cursor.
         \return The key.
         \exception tokyo::Exception When fetching the key fails.
         */
        Key cursor_key(BDBCUR *cur) {
            int sz = 0;
            Value *ptr = static_cast<Value *>(tcbdbcurkey3(cur, &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }

        //! Get the cursors value.
        /*!
         \param cur The cursor.
         \return The value.
         \exception tokyo::Exception When fetching the key fails.
         */
        Value cursor_value(BDBCUR *cur) {
            int sz = 0;
            Value *ptr = static_cast<Value *>(tcbdbcurval3(cur, &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }
        
        //! Begin a transaction.
        void begin_transaction() {
            if(!tcbdbtranbegin(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Commit a transaction.
        void commit_transaction() {
            if(!tcbdbtrancommit(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Rollback a transaction.
        void abort_transaction() {
            if(!tcbdbtranabort(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
    };
    
    //! DB Object to store full records.
    /*!
     Template specialization for string keys and unsigned long long data.
     \par Wraps a Tokyo Cabinet B+ Tree database. Provides the _put method
     to store to the database. Implementations should inherit from this class,
     and add members for the different indexes required.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<>
    class DB <std::string, unsigned long long> {
    protected:
        //! The B+ tree database object.
        TCBDB *_db;
        
        //! Method for opening the database.
        void (*_open_func)(TCBDB *, int);
        
        //! Mode to open the database in.
        int _mode;
    public:
        //! Create and open the DB object.
        /*!
         \param db_open_func Method to use when opening the database.
         \param mode Mode flags to pass to the db open method.
         */
        DB(void (*db_open_func)(TCBDB *, int), int mode) {
            _open_func = db_open_func;
            _mode = mode;
            _db = tcbdbnew();
            (*_open_func)(_db, _mode);
        }
        
        //! Destructor
        virtual ~DB() {
            tcbdbclose(_db);
            tcbdbdel(_db);
        }
        
        //! Get the record associated with the primary key.
        /*!
         The value returned is allocated with malloc, and must be freed.
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key to get.
         \return The record associated with the primary key.
         */
        unsigned long long _at(const std::string &key) {
            int sz = 0;
            unsigned long long *ptr = static_cast<unsigned long long *>(tcbdbget(_db, key.c_str(), key.size(), &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }
        
        //! Store a record into the database.
        /*!
         If the record already exists, it is replaced.
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key for the record.
         \param value The record to store.
         \exception Exception When unable to put the record in the database.
         */
        void _put(const std::string &key, const unsigned long long value) {
            if(!tcbdbput(_db, key.c_str(), key.size(), &value, sizeof(unsigned long long)))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Remove a record from the database.
        /*!
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key for the record.
         \exception Exception When unable to remove the record.
         */
        void _remove(const std::string &key) {
            if(!tcbdbout(_db, key.c_str(), key.size()))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Get the maximum primary key value in the database.
        /*!
         \return The max primary key in the DB.
         \exception Exception When a database exception occurs.
         */
        std::string max() {
            BDBCUR *cur = end();        
            
            int sz = 0;
            const char *ptr = static_cast<const char *>(tcbdbcurkey3(cur, &sz));
            tcbdbcurdel(cur);
            
            if(!ptr || !sz) {
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return std::string(ptr);
        }
        
        //! Get the minimum primary key value in the database.
        /*!
         \return the min primary key in the DB.
         \exception Exception When a database exception occurs.
         */
        std::string min() {
            BDBCUR *cur = begin();
            
            int sz = 0;
            const char *ptr = static_cast<const char *>(tcbdbcurkey3(cur, &sz));
            tcbdbcurdel(cur);
            
            if(!ptr || !sz) {
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return std::string(ptr);
        }
        
        //! Return the number of records in the database.
        /*!
         \return Number of records.
         */
        inline unsigned long long size() {
            return tcbdbrnum(_db);
        }
        
        //! Get a cursor at a primary key.
        /*!
         \param key The primary key to place the cursor at.
         \return The cursor.
         \exception Exception If there was a database error.
         */
        BDBCUR *cursor(const std::string &key) {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurjump(cur, key.c_str(), key.size())) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get a cursor at the first primary key.
        /*!
         \return The cursor at the first (min) primary key.
         \exception Exception If there was a database error.
         */
        BDBCUR *begin() {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurfirst(cur)) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get a cursor at the last primary key.
        /*!
         \return The cursor at the last (max) primary key.
         \exception Exception If there was a database error.
         */
        BDBCUR *end() {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurlast(cur)) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get the cursors key.
        /*!
         \param cur The cursor.
         \return The key.
         \exception tokyo::Exception When fetching the key fails.
         */
        std::string cursor_key(BDBCUR *cur) {
            int sz = 0;
            const char *ptr = static_cast<const char *>(tcbdbcurkey3(cur, &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return std::string(ptr);
        }

        //! Get the cursors value.
        /*!
         \param cur The cursor.
         \return The value.
         \exception tokyo::Exception When fetching the key fails.
         */
        unsigned long long cursor_value(BDBCUR *cur) {
            int sz = 0;
            const unsigned long long *ptr = static_cast<const unsigned long long *>(tcbdbcurval3(cur, &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }        
        
        //! Begin a transaction.
        void begin_transaction() {
            if(!tcbdbtranbegin(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Commit a transaction.
        void commit_transaction() {
            if(!tcbdbtrancommit(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Rollback a transaction.
        void abort_transaction() {
            if(!tcbdbtranabort(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
    };    

    //! DB Object to store full records.
    /*!
     Template specialization for unsigned long long keys and string data.
     \par Wraps a Tokyo Cabinet B+ Tree database. Provides the _put method
     to store to the database. Implementations should inherit from this class,
     and add members for the different indexes required.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<>
    class DB <unsigned long long, std::string> {
    protected:
        //! The B+ tree database object.
        TCBDB *_db;
        
        //! Method for opening the database.
        void (*_open_func)(TCBDB *, int);
        
        //! Mode to open the database in.
        int _mode;
    public:
        //! Create and open the DB object.
        /*!
         \param db_open_func Method to use when opening the database.
         \param mode Mode flags to pass to the db open method.
         */
        DB(void (*db_open_func)(TCBDB *, int), int mode) {
            _open_func = db_open_func;
            _mode = mode;
            _db = tcbdbnew();
            (*_open_func)(_db, _mode);
        }
        
        //! Destructor
        virtual ~DB() {
            tcbdbclose(_db);
            tcbdbdel(_db);
        }
        
        //! Get the record associated with the primary key.
        /*!
         The value returned is allocated with malloc, and must be freed.
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key to get.
         \return The record associated with the primary key.
         */
        std::string _at(const unsigned long long key) {
            int sz = 0;
            const char *ptr = static_cast<const char *>(tcbdbget3(_db, &key, sizeof(unsigned long long), &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return std::string(ptr);
        }
        
        //! Store a record into the database.
        /*!
         If the record already exists, it is replaced.
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key for the record.
         \param value The record to store.
         \exception Exception When unable to put the record in the database.
         */
        void _put(const unsigned long long key, const std::string &value) {
            if(!tcbdbput(_db, &key, sizeof(unsigned long long), value.c_str(), value.size()))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Remove a record from the database.
        /*!
         \par This method starts with an underscore to avoid inheritence and overloading issues.
         \param key The primary key for the record.
         \exception Exception When unable to remove the record.
         */
        void _remove(const unsigned long long key) {
            if(!tcbdbout(_db, &key, sizeof(unsigned long long)))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Get the maximum primary key value in the database.
        /*!
         \return The max primary key in the DB.
         \exception Exception When a database exception occurs.
         */
        unsigned long long max() {
            BDBCUR *cur = end();        
            
            int sz = 0;
            const unsigned long long *ptr = static_cast<const unsigned long long *>(tcbdbcurkey3(cur, &sz));
            tcbdbcurdel(cur);
            
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }
        
        //! Get the minimum primary key value in the database.
        /*!
         \return the min primary key in the DB.
         \exception Exception When a database exception occurs.
         */
        unsigned long long min() {
            BDBCUR *cur = begin();
            
            int sz = 0;
            const unsigned long long *ptr = static_cast<const unsigned long long *>(tcbdbcurkey3(cur, &sz));
            tcbdbcurdel(cur);
            
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }
        
        //! Return the number of records in the database.
        /*!
         \return Number of records.
         */
        inline unsigned long long size() {
            return tcbdbrnum(_db);
        }
        
        //! Get a cursor at a primary key.
        /*!
         \param key The primary key to place the cursor at.
         \return The cursor.
         \exception Exception If there was a database error.
         */
        BDBCUR *cursor(const unsigned long long key) {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurjump(cur, &key, sizeof(unsigned long long))) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get a cursor at the first primary key.
        /*!
         \return The cursor at the first (min) primary key.
         \exception Exception If there was a database error.
         */
        BDBCUR *begin() {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurfirst(cur)) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get a cursor at the last primary key.
        /*!
         \return The cursor at the last (max) primary key.
         \exception Exception If there was a database error.
         */
        BDBCUR *end() {
            BDBCUR *cur = tcbdbcurnew(_db);
            if(!tcbdbcurlast(cur)) {
                tcbdbcurdel(cur);
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            }
            return cur;
        }
        
        //! Get the cursors key.
        /*!
         \param cur The cursor.
         \return The key.
         \exception tokyo::Exception When fetching the key fails.
         */
        unsigned long long cursor_key(BDBCUR *cur) {
            int sz = 0;
            const unsigned long long *ptr = static_cast<const unsigned long long *>(tcbdbcurkey3(cur, &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return *ptr;
        }

        //! Get the cursors value.
        /*!
         \param cur The cursor.
         \return The value.
         \exception tokyo::Exception When fetching the key fails.
         */
        std::string cursor_value(BDBCUR *cur) {
            int sz = 0;
            const char *ptr = static_cast<const char *>(tcbdbcurval3(cur, &sz));
            if(!ptr || !sz)
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
            return std::string(ptr);
        }

        //! Begin a transaction.
        void begin_transaction() {
            if(!tcbdbtranbegin(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Commit a transaction.
        void commit_transaction() {
            if(!tcbdbtrancommit(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
        
        //! Rollback a transaction.
        void abort_transaction() {
            if(!tcbdbtranabort(_db))
                throw Exception("DB error", tcbdberrmsg(tcbdbecode(_db)));
        }
    };
        
    //! Index object for allowing searches and ordering.
    /*!
     A DB object should create an index for each search it will need to perform.
     \par A specialized version of this template has been created for strings.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<typename Key, typename Value>
    class Index : public DB<Value, Key> {
    public:
        //! Create a new Index.
        /*!
         \param db_open_func Method to use when opening the database.
         \param mode Mode flags to pass to the db open method.
         */
        Index(void (*db_open_func)(TCBDB *, int), int mode) : DB<Key, Value>(db_open_func, mode) {
        }
        
        //! Destructor
        virtual ~Index() {
        }
        
        //! Put the value and key into the index.
        /*!
         Duplicate records are inserted in the index as additional nodes.
         \param a The value to index on
         \param k The primary key associated with the value.
         \exception Exception If the value could not be written.
         \warning This class does not use the _put method, because it uses different
            logic for duplicate keys. If your usage of the index depends on
            unique constraints, use the _put version.
         \sa DB::_put()
         */
        virtual void put(const Value &a, const Key &k) {
            if(!tcbdbputdup(this->_db, &a, sizeof(Value), &k, sizeof(Key)))
                throw Exception("Index error", tcbdberrmsg(tcbdbecode(this->_db)));
        }
        
        //! Remove the value and key from the index.
        /*!
         Only the record with the matching key is removed from the index.
         \param a The value indexed.
         \param k The key associated with the database.
         */
        virtual void remove(const Value &a, const Key &k) {
            BDBCUR *cur = cursor(a);

            do {
                int sz;
                const Value *value = static_cast<const Value *>(tcbdbcurkey3(this->_db, &sz));
                if(!(*value == a)) break;
                const Key *key = static_cast<const Key *>(tcbdbcurval3(this->_db, &sz));
                if(!(*key == k))
                    tcbdbcurout(cur);
            } while(tcbdbcurnext(cur));
            
            tcbdbcurdel(cur);
        }
        
        //! Between query on the index.
        /*!
         Get a set of keys that exist between two values.
         \param a Lower bound, inclusive.
         \param b Higher bound, exclusive.
         \return Set of keys that match. An empty set if no keys matched.
         \exception Exception When the index cannot be read.
         \warning If a must be lower than b, the command will switch the order of
            the arguments if they are not a < b.
         */
        std::set<Key> between(const Value &a, const Value &b) {
            // Make sure a is less than b.
            if(a > b)
                return between(b, a);
            
            // Create a cursor so we can iterate.
            BDBCUR *cur = tcbdbcurnew(this->_db);
            if(!tcbdbcurjump(cur, &a, sizeof(Value))) {
                tcbdbcurdel(cur);
                throw Exception("Index error", tcbdberrmsg(tcbdbecode(this->_db)));
            }
            
            std::set<Key> results;
            int sz = 0;
            do {
                // Get the key, to ensure we are still between.
                const Value *ptr = static_cast<const Value *>(tcbdbcurkey3(cur, &sz));
                if(!ptr || *ptr > b || *ptr == b) break;
                
                // get the value (a list of type Key).
                Key *keys = static_cast<Key *>(tcbdbcurval3(cur, &sz));
                if(!keys) break;
                if(!sz) continue;
                
                // Loop over the keys and build the results.
                int count = sz / sizeof(Key);
                for(int h = 0; h < count; ++h, ++keys)
                    results.insert(*keys);
            } while(tcbdbcurnext(cur));
            
            tcbdbcurdel(cur);
            return results;
        }
        
        //! Is query on the index.
        /*!
         \param a value to look for.
         \return Set of keys that match. An empty set if no keys matched.
         */
        std::set<Key> is(const Value &a) {
            // Get a list of all matching index entries.
            TCLIST *values = tcbdbget4(this->_db, &a, sizeof(Value));
            if(!values)
                return std::set<Key>();

            // Loop over each index entry and convert it into the response set.
            std::set<Key> results;
            while(true) {
                int sz = 0;
                Value *keys = static_cast<Value *>(tclistshift(values, &sz));
                if(!keys) break;
                
                int count = sz / sizeof(Key);
                for(int h = 0; h < count; ++h, ++keys)
                    results.insert(*keys);
                
                // Must free keys after each loop.
                free(keys);
            }
            
            tclistdel(values);
            return results;
        }
    };

    //! Specialized Index class for allowing searches on strings.
    /*!
     A DB object should create an index for each search it will need to perform.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template <>
    class Index <unsigned long long, std::string> : public DB<std::string, unsigned long long> {
    public:
        //! Create a new Index.
        /*!
         \param db_open_func Method to use when opening the database.
         \param mode Mode flags to pass to the db open method.
         */
        Index(void (*db_open_func)(TCBDB *, int), int mode) : DB<std::string, unsigned long long>(db_open_func, mode) {
        }
        
        //! Delete an Index.
        ~Index() {
        }

        //! Put the value and key into the index.
        /*!
         Duplicate records are inserted in the index as additional nodes.
         \param a The value to index on
         \param k The primary key associated with the value.
         \exception Exception If the value could not be written.
         \warning This class does not use the _put method, because it uses different
         logic for duplicate keys. If your usage of the index depends on
         unique constraints, use the _put version.
         \sa DB::_put()
         */
        virtual void put(const std::string &a, const unsigned long long k) {
            if(!tcbdbputdup(this->_db, a.c_str(), a.size(), &k, sizeof(unsigned long long)))
                throw Exception("Index error", tcbdberrmsg(tcbdbecode(this->_db)));
        }
        
        //! Remove the value and key from the index.
        /*!
         Only the record with the matching key is removed from the index.
         \param a The value indexed.
         \param k The key associated with the database.
         */
        virtual void remove(const std::string &a, const unsigned long long k) {
            BDBCUR *cur = cursor(a);
            
            do {
                int sz;
                const char *value = static_cast<const char *>(tcbdbcurkey3(cur, &sz));
                if(a.compare(value) != 0) break;
                const unsigned long long *key = static_cast<const unsigned long long *>(tcbdbcurval3(cur, &sz));
                if(*key == k) {
                    tcbdbcurout(cur);
                    break;
                }
            } while(tcbdbcurnext(cur));
            
            tcbdbcurdel(cur);
        }
        
        //! Between query on the index.
        /*!
         Get a set of keys that exist between two values.
         \param a Lower bound, inclusive.
         \param b Higher bound, exclusive.
         \return Set of keys that match. An empty set if no keys matched.
         \exception Exception When the index cannot be read.
         \warning If a must be lower than b, the command will switch the order of
            the arguments if they are not a < b.
         */
        std::set<unsigned long long> between(const std::string &a, const std::string &b) {
            // Make sure a is less than b.
            if(a.compare(b) > 0)
                return between(b, a);
            
            // Create a cursor so we can iterate.
            BDBCUR *cur = cursor(a);
            std::set<unsigned long long> results;
            int sz = 0;
            do {
                // Get the key, to ensure we are still between.
                const char *ptr = static_cast<const char *>(tcbdbcurkey3(cur, &sz));
                if(!ptr || b.compare(ptr) >= 0) break;
                
                // get the value (a list of type Key).
                unsigned long long *keys = const_cast<unsigned long long *>(static_cast<const unsigned long long *>(tcbdbcurval3(cur, &sz)));
                if(!keys) break;
                if(!sz) continue;
                
                // Loop over the keys and build the results.
                int count = sz / sizeof(unsigned long long);
                for(int h = 0; h < count; ++h, ++keys)
                    results.insert(*keys);
            } while(tcbdbcurnext(cur));
            
            tcbdbcurdel(cur);
            
            return results;
        }
        
        //! Is query on the index.
        /*!
         \param a value to look for.
         \return Set of keys that match. An empty set if no keys matched.
         */
        std::set<unsigned long long> is(const std::string &a) {
            // Get a list of all matching index entries.
            TCLIST *values = tcbdbget4(_db, a.c_str(), a.size());
            if(!values)
                return std::set<unsigned long long>();
            
            // Loop over each index entry and convert it into the response set.
            std::set<unsigned long long> results;
            while(true) {
                // shift value off of list.
                int sz = 0;
                unsigned long long *keys_ptr = static_cast<unsigned long long *>(tclistshift(values, &sz));
                if(!keys_ptr) break;
                
                // Loop over array of values on list.
                int count = sz / sizeof(unsigned long long);
                unsigned long long *keys = keys_ptr;
                for(int h = 0; h < count; ++h, ++keys) {
                    results.insert(*keys);
                }
                
                // Must free keys after each loop.
                free(keys_ptr);
            }            
            
            tclistdel(values);
            return results;
        }
        
        //! Starts query on the index.
        /*!
         \param a value to look for.
         \return Set of keys that match. An empty set if no keys matched.
         */
        std::set<unsigned long long> starts(const std::string &a) {
            TCLIST *values = tcbdbfwmkeys2(_db, a.c_str(), -1);
            if(!values) {
                return std::set<unsigned long long>();
            }
            
            std::set<unsigned long long> results;
            while(true) {
                // shift value off of list.
                int sz = 0;
                char *value_ptr = static_cast<char *>(tclistshift(values, &sz));
                if(!value_ptr) break;

                // Get the keys associated with an index value.
                TCLIST *key_list = tcbdbget4(_db, value_ptr, sz);
                while(true) {
                    // Loop until the list of keys is empty.
                    unsigned long long *keys_ptr = static_cast<unsigned long long *>(tclistshift(key_list, &sz));
                    if(!keys_ptr) break;
                    
                    // Loop over each key array and add the keys to the set.
                    int count = sz / sizeof(unsigned long long);
                    unsigned long long *keys = keys_ptr;
                    for(int h = 0; h < count; ++h, ++keys) {
                        results.insert(*keys);
                    }
                    
                    // Clean up after the list shift.
                    free(keys_ptr);
                }
                // Clean up after the tcbdbget4
                tclistdel(key_list);
                
                // Must free keys after each loop.
                free(value_ptr);
            }
            
            tclistdel(values);
            return results;
        }
    };

    //! Full text index.
    /*!
     Wraps a Tokyo DB full text index object. A DB object should create an index for
        each search it will need to perform.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<typename Key=unsigned long long>
    class Search {
    protected:
        //! The Full text database object.
        TCIDB *_db;
        
        //! Method for opening the database.
        void (*_open_func)(TCIDB *, int);
        
        //! Mode to open the database in.
        int _mode;
    public:
        //! Create and open the full-text search object.
        /*!
         \param db_open_func Method to use when opening the database.
         \param mode Mode flags to pass to the db open method.
         */
        Search(void (*db_open_func)(TCIDB *, int), int mode) {
            _open_func = db_open_func;
            _mode = mode;
            _db = tcidbnew();
            (*_open_func)(_db, _mode);
        }
        
        //! Close and Delete the DB object.
        ~Search() {
            tcidbclose(_db);
            tcidbdel(_db);
        }
        
        //! Index the the given text and key.
        /*!
         \param a The text to index.
         \param k The key associated with the text.
         \exception If the record cannot be written.
         */
        void index(const std::string &a, const Key &k) {
            if(!tcidbput(_db, k, a.c_str()))
                throw Exception("Search error", tcidberrmsg(tcidbecode(_db)));
        }
        
        //! Remove the index record for a specific key.
        /*!
         \param k The key to remove.
         \exception When the index cannot be removed.
         */
        void remove(const Key &k) {
            if(!tcidbout(_db, k))
                throw Exception("Search error", tcidberrmsg(tcidbecode(_db)));
        }
        
        //! Re-optimize the index.
        /*!
         Re-index the database after multiple modifications.
         \exception When the index cannot be optimized.
         */
        void optimize() {
            if(!tcidboptimize(_db))
                throw Exception("Search error", tcidberrmsg(tcidbecode(_db)));
        }
        
        //! Like query on the index.
        /*!
         The compound expression syntax can be seen on at
            http://tokyocabinet.sourceforge.net/dystopiadoc/#dystopiaapi
         \param query The compound query to perform. 
         \return the set of matching keys. An empty set if no keys matched.
         */
        std::set<Key> like(const std::string &query) {
            std::set<Key> results;
            like(query, results);
            return results;
        }

        //! Like query on the index.
        /*!
         The compound expression syntax can be seen on at
            http://tokyocabinet.sourceforge.net/dystopiadoc/#dystopiaapi
         \param query The compound query to perform. 
         \param results Where to store the results.
         */
        void like(const std::string &query, std::set<Key> &results) {
            int sz;
            unsigned long long *matches = tcidbsearch2(_db, query.c_str(), &sz);
            if(!matches || !sz) {
                free(matches);
                return;
            }
            
            for(int h = 0; h < sz; ++h) {
                results.insert(static_cast<Key>(matches[h]));
            }
            free(matches);
        }
    };

    //! Tag Index.
    /*!
     Wraps a Tokyo tag index object. An index object should be created  for
     each search.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    template<typename Key>
    class Tags {
    protected:
        //! The Full text database object.
        TCJDB *_db;
        
        //! Method for opening the database.
        void (*_open_func)(TCJDB *, int);
        
        //! Mode to open the database in.
        int _mode;
    public:
        //! Create and open the DB object.
        /*!
         \param db_open_func Method to use when opening the database.
         \param mode Mode flags to pass to the db open method.
         */
        Tags(void (*db_open_func)(TCJDB *, int), int mode) {
            _open_func = db_open_func;
            _mode = mode;

            _db = tcjdbnew();
            (*_open_func)(_db, _mode);
        }
        
        //! Close and Delete the DB object.
        ~Tags() {
            tcjdbclose(_db);
            tcjdbdel(_db);
        }
        
        //! Index the the given tags and key.
        /*!
         \param a The tags to index.
         \param k The key associated with the tags.
         \exception If the record cannot be written.
         */
        void index(const std::set<std::string> &a, const Key &k) {
            TCLIST *tags = tclistnew2(a.size());
            
            for(std::set<std::string>::const_iterator iter = a.begin(); iter != a.end(); ++iter)
                tclistpush2(tags, iter->c_str());
            
            if(!tcjdbput(_db, k, tags)) {
                tclistdel(tags);
                throw Exception("Tag error", tcjdberrmsg(tcjdbecode(_db)));
            }
            
            tclistdel(tags);
        }
        
        //! Remove the index record for a specific key.
        /*!
         \param k The key to remove.
         \exception When the index cannot be removed.
         */
        void remove(const Key &k) {
            if(!tcjdbout(_db, k))
                throw Exception("Tag error", tcjdberrmsg(tcjdbecode(_db)));
        }
        
        //! Re-optimize the index.
        /*!
         Re-index the database after multiple modifications.
         \exception When the index cannot be optimized.
         */
        void optimize() {
            if(!tcjdboptimize(_db))
                throw Exception("Tag error", tcjdberrmsg(tcjdbecode(_db)));
        }
        
        //! Like query on the tag index.
        /*!
         The compound expression syntax can be seen on at
            http://tokyocabinet.sourceforge.net/dystopiadoc/#laputaapi
         \param query The compound query to perform. 
         \return the set of matching keys. An empty set if no keys matched.
         \sa http://tokyocabinet.sourceforge.net/dystopiadoc/#laputaapi
         */
        std::set<Key> tagged(const std::string &query) {
            std::set<Key> results;
            tagged(query, results);
            return results;
        }
        
        //! Like query on the tag index.
        /*!
         The compound expression syntax can be seen on at
         http://tokyocabinet.sourceforge.net/dystopiadoc/#laputaapi
         \param query The compound query to perform. 
         \param results Set to store the matching keys.
         \sa http://tokyocabinet.sourceforge.net/dystopiadoc/#laputaapi
         */
        void tagged(const std::string &query, std::set<Key> &results) {
            int sz;
            unsigned long long *matches;
            
            matches = tcjdbsearch2(_db, query.c_str(), &sz);
            if(!matches || !sz) {
                free(matches);
                return;
            }
            
            for(int h = 0; h < sz; ++h) {
                results.insert(static_cast<Key>(matches[h]));
            }            
            free(matches);
        }
    };
};
