/*
 \file Tokyo.cpp
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

#include "build/default/config.h"
#include "Tokyo.h"

namespace tokyo {
    namespace {
        bool arrays_equal(const void * const a,
                          size_t a_len,
                          const void * const b,
                          size_t b_len) {
            if(a_len != b_len) return false;
            for(unsigned int h = 0; h < b_len; ++h)
                if(((const char * const)a)[h] ^ ((const char * const)b)[h])
                    return false;
            return true;
        }
    };
    
    DB::DB() {
    }
    
    DB::~DB() {
    }
    
    //=====================================================================
    // TreeDB Implementation
    //=====================================================================
    
    TreeDB::TreeDB(const std::string &filename,
                   const int mode,
                   void (*db_tune_func)(TCBDB *)) {
        _db = tcbdbnew();
        if(db_tune_func)
            (*db_tune_func)(db());
        tcbdbopen(db(), filename.c_str(), mode);
    }
    
    TreeDB::~TreeDB() {
        tcbdbclose(_db);
        tcbdbdel(_db);
        _db = 0;
    }
    
    DB::value_t TreeDB::at(const void *key, const size_t len) {
        int sz = 0;
        void *ptr = tcbdbget(db(), key, len, &sz);
        if(!ptr || !sz)
            ptr = 0;
        return value_t(ptr, sz);
    }
    
    bool TreeDB::at_together(const void *key,
                             const size_t len,
                             list_value_t &results) {
        TCLIST *ptr = tcbdbget4(db(), key, len);
        if(!ptr) {
            tclistdel(ptr);
            return false;
        }
        
        void *item;
        int sz;
        while(tclistnum(ptr)) {
            item = tclistshift(ptr, &sz);
            if(!item || !sz)
                continue;
            results.push_back(value_t(item,sz));
        }
        tclistdel(ptr);
        return true;
    }
    
    bool TreeDB::at_range(const void *start,
                          const size_t start_len,
                          const bool start_inc,
                          const void *end,
                          const size_t end_len,
                          const bool end_inc,
                          list_value_t &results) {
        list_value_t keys;
        if(range_keys(start,
                      start_len,
                      start_inc,
                      end,
                      end_len,
                      end_inc,
                      keys)) {
            for(list_value_t::const_iterator iter = keys.begin();
                iter != keys.end();
                ++iter) {
                at_together(iter->first, iter->second, results);
                free(iter->first);
            }
            return true;
        }
        return false;
    }
    
    void TreeDB::place(const void *key,
                       const size_t key_len,
                       const void * const val, 
                       const size_t val_len) {
        if(!tcbdbput(db(), key, key_len, val, val_len))
            throw Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::place_with_existing(const void *key,
                                     const size_t key_len,
                                     const void * const val,
                                     const size_t val_len) {
        if(!tcbdbputdup(db(), key, key_len, val, val_len))
            throw Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::place_together(const void *key,
                                const size_t key_len,
                                const list_value_t &vals) {
        // XXX This should probably be modified to build a TCLIST, then call
        // a different method.
        for(list_value_t::const_iterator val = vals.begin();
            val != vals.end();
            ++val) {
            place_with_existing(key, key_len, val->first, val->second);
        }
    }
    
    void TreeDB::place_if_absent(const void *key, 
                                 const size_t key_len, 
                                 const void * const val,
                                 const size_t val_len) {
        if(!tcbdbputkeep(db(), key, key_len, val, val_len))
            throw Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::place_or_append(const void *key, 
                                 const size_t key_len, 
                                 const void * const val,
                                 const size_t val_len) {
        if(!tcbdbputcat(db(), key, key_len, val, val_len))
            throw Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::remove(const void *key, const size_t len) {
        if(!tcbdbout(db(), key, len))
            throw Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::remove_together(const void *key, const size_t len) {
        if(!tcbdbout3(db(), key, len))
            throw Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::remove_from_existing(const void *key,
                                      const size_t len,
                                      const void * const val,
                                      const size_t val_len) {
        list_value_t values;
        at_together(key, len, values);
        
        bool rewrite = false;
        for(list_value_t::iterator iter = values.begin();
            iter != values.end();
            ++iter) {
            if(arrays_equal(val, val_len, iter->first, iter->second)) {
                free(iter->first);
                values.erase(iter);
                rewrite = true;
                break;
            }
        }
        
        if(rewrite) {
            remove_together(key, len);
            place_together(key, len, values);
        }
        
        for(list_value_t::const_iterator iter = values.begin();
            iter != values.end();
            ++iter) {
            free(iter->first);
        }
    }
    
    void TreeDB::start_writes() {
        if(!tcbdbtranbegin(db()))
            throw Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::save_writes() {
        if(!tcbdbtrancommit(db()))
            throw Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
    }
    
    void TreeDB::abort_writes() {
        if(!tcbdbtranabort(db()))
            throw Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
    }
    
    DB::value_t TreeDB::max_key() {
        BDBCUR *cur = tcbdbcurnew(db());
        if(!tcbdbcurlast(cur)) {
            tcbdbcurdel(cur);
            unsigned long long *ptr = (unsigned long long *)malloc(sizeof(unsigned long long));
            *ptr = 0;
            return value_t(ptr, sizeof(unsigned long long));
        }
        
        int sz = 0;
        void *ptr = tcbdbcurkey(cur, &sz);
        tcbdbcurdel(cur);
        if(!ptr || !sz)
            throw Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
        
        return value_t(ptr, sz);
    }
    
    DB::value_t TreeDB::min_key() {
        BDBCUR *cur = tcbdbcurnew(db());
        if(!tcbdbcurfirst(cur)) {
            tcbdbcurdel(cur);
            unsigned long long *ptr = (unsigned long long *)malloc(sizeof(unsigned long long));
            *ptr = 0;
            return value_t(ptr, sizeof(unsigned long long));
        }
        
        int sz = 0;
        void *ptr = tcbdbcurkey(cur, &sz);
        tcbdbcurdel(cur);
        
        if(!ptr || !sz)
            throw Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
        
        return value_t(ptr, sz);
    }
    
    bool TreeDB::range_keys(const void *start,
                            const size_t start_len,
                            const bool start_inc,
                            const void *end,
                            const size_t end_len,
                            const bool end_inc,
                            list_value_t &keys) {
        TCLIST *ptr = tcbdbrange(db(),
                                 start,
                                 start_len,
                                 start_inc,
                                 end,
                                 end_len,
                                 end_inc,
                                 -1);
        if(!ptr) {
            tclistdel(ptr);
            return false;
        }
        
        void *key;
        int key_sz;
        while(tclistnum(ptr)) {
            key = tclistshift(ptr, &key_sz);
            if(!key || !key_sz)
                continue;
            keys.push_back(value_t(key, key_sz));
        }
        tclistdel(ptr);
        return true;
    }
    
    //=====================================================================
    // TextSearcher Implementation
    //=====================================================================
    
    TextSearcher::TextSearcher(const std::string &filename,
                               const int mode,
                               void (*db_tune_func)(TCQDB *)) {
        _db = tcqdbnew();
        if(db_tune_func)
            (*db_tune_func)(db());
        tcqdbopen(db(), filename.c_str(), mode);
    }
    
    TextSearcher::~TextSearcher() {
        tcqdbclose(_db);
        tcqdbdel(_db);
        _db = NULL;
    }
    
    void TextSearcher::index(const key_t key, const value_t &txt) {
        if(!tcqdbput(db(), key, txt.c_str()))
            throw Exception("TextSearcherError", tcqdberrmsg(tcqdbecode(db())));
    }
    
    void TextSearcher::remove(const key_t key, const value_t &txt) {
        if(!tcqdbout(db(), key, txt.c_str()))
            throw Exception("TextSearcherError", tcqdberrmsg(tcqdbecode(db())));
    }
    
    bool TextSearcher::search(const std::string &query,
                              set_key_t &results) {
        int sz = 0;
        key_t *ptr = tcqdbsearch(db(),
                                 query.c_str(),
                                 QDBSSUBSTR,
                                 &sz);
        if(!ptr)
            return false;
        
        for(int h = 0; h < sz; ++h) {
            results.insert(*(ptr++));
        }
        return true;
    }
    
    void TextSearcher::optimize() {
        if(!tcqdboptimize(db()))
            throw Exception("TextSearcherError", tcqdberrmsg(tcqdbecode(db())));
    }
    
    void TextSearcher::truncate() {
        if(!tcqdbvanish(db()))
            throw Exception("TextSearcherError", tcqdberrmsg(tcqdbecode(db())));
    }
    
    //=====================================================================
    // TagSearcher Implementation
    //=====================================================================
    
    TagSearcher::TagSearcher(const std::string &filename,
                       const int mode,
                       void (*db_tune_func)(TCWDB *)) {
        _db = tcwdbnew();
        if(db_tune_func)
            (*db_tune_func)(db());
        tcwdbopen(db(), filename.c_str(), mode);
    }
    
    TagSearcher::~TagSearcher() {
        tcwdbclose(_db);
        tcwdbdel(_db);
        _db = NULL;
    }
    
    void TagSearcher::index(const key_t key,
                            const value_t &txt) {
        set_value_t words;
        words.insert(txt);
        index(key, words);
    }
    
    void TagSearcher::index(const key_t key,
                            const set_value_t &words) {
        TCLIST *l = tclistnew();
        for(set_value_t::const_iterator iter = words.begin();
            iter != words.end();
            ++iter) {
            tclistpush2(l, iter->c_str());
        }
        if(!tcwdbput(db(), key, l)) {
            tclistdel(l);
            throw Exception("TagSearcherError", tcwdberrmsg(tcwdbecode(db())));
        }
        tclistdel(l);
    }
    
    void TagSearcher::remove(const key_t key,
                             const value_t &txt) {
        set_value_t words;
        words.insert(txt);
        remove(key, words);
    }
    
    void TagSearcher::remove(const key_t key,
                             const set_value_t &words) {
        TCLIST *l = tclistnew();
        for(set_value_t::const_iterator iter = words.begin();
            iter != words.end();
            ++iter) {
            tclistpush2(l, iter->c_str());
        }
        if(!tcwdbout(db(), key, l)) {
            tclistdel(l);
            throw Exception("TagSearcherError", tcwdberrmsg(tcwdbecode(db())));
        }
        tclistdel(l);
    }
    bool TagSearcher::search(const std::string &query,
                             set_key_t &results) {
        int sz = 0;
        unsigned long long *ptr = tcwdbsearch(db(),
                                              query.c_str(),
                                              &sz);
        if(!ptr)
            return false;
        
        for(int h = 0; h < sz; ++h) {
            results.insert(*(ptr++));
        }
        return false;
    }
    
    void TagSearcher::optimize() {
        if(!tcwdboptimize(db()))
            throw Exception("TagSearcherError", tcwdberrmsg(tcwdbecode(db())));
    }
    
    void TagSearcher::truncate() {
        if(!tcwdbvanish(db()))
            throw Exception("TagSearcherError", tcwdberrmsg(tcwdbecode(db())));
    }
};