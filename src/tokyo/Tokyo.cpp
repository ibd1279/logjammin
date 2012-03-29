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

#include "tokyo/Tokyo.h"
#include <iostream>

using lj::Exception;

namespace
{
    bool arrays_equal(const void * const a,
                      size_t a_len,
                      const void * const b,
                      size_t b_len)
    {
        if (a_len != b_len)
        {
            return false;
        }
        for (unsigned int h = 0; h < b_len; ++h)
        {
            if (((const char * const)a)[h] ^ ((const char * const)b)[h])
            {
                return false;
            }
        }
        return true;
    }
};

namespace tokyo
{    
    DB::DB()
    {
    }
    
    DB::~DB()
    {
    }
    
    //=====================================================================
    // Hash_db Implementation
    //=====================================================================
    const std::string Hash_db::k_db_type("Hash");
    
    Hash_db::Hash_db(const std::string& filename,
                     const int mode,
                     Hash_db::Tune_function_pointer db_tune_func,
                     const void* ptr)
    {
        db_ = tchdbnew();
        if (db_tune_func)
        {
            (*db_tune_func)(db(), ptr);
        }
        tchdbopen(db(), filename.c_str(), mode);
    }
    
    Hash_db::~Hash_db()
    {
        tchdbclose(db_);
        tchdbdel(db_);
        db_ = 0;
    }
    
    DB::value_t Hash_db::at(const void* key, const size_t len)
    {
        int sz = 0;
        void* ptr = tchdbget(db(), key, len, &sz);
        if (!ptr || !sz)
        {
            ptr = 0;
        }
        return value_t(ptr, sz);
    }
    
    void Hash_db::place(const void* key,
                        const size_t key_len,
                        const void* const val, 
                        const size_t val_len)
    {
        if (!tchdbput(db(), key, key_len, val, val_len))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::place_if_absent(const void* key, 
                                  const size_t key_len, 
                                  const void* const val,
                                  const size_t val_len)
    {
        if (!tchdbputkeep(db(), key, key_len, val, val_len))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::place_or_append(const void*key, 
                                  const size_t key_len, 
                                  const void* const val,
                                  const size_t val_len)
    {
        if (!tchdbputcat(db(), key, key_len, val, val_len))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::remove(const void* key, const size_t len)
    {
        if (!tchdbout(db(), key, len))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::start_writes()
    {
        if (!tchdbtranbegin(db()))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::save_writes()
    {
        if (!tchdbtrancommit(db()))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::abort_writes()
    {
        if (!tchdbtranabort(db()))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    long long Hash_db::count()
    {
        return (long long)tchdbrnum(db());
    }
    
    void Hash_db::truncate()
    {
        if (!tchdbvanish(db()))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::copy(const std::string &target)
    {
        if (!tchdbcopy(db(), target.c_str()))
        {
            throw new Exception("TokyoHash", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    void Hash_db::optimize()
    {
        long long bnum = count() * 2;
        if (!tchdboptimize(db(), bnum, -1, -1, 0xFF))
        {
            throw new Exception("TokyoHashOptimize", tchdberrmsg(tchdbecode(db())));
        }
    }
    
    //=====================================================================
    // Tree_db Implementation
    //=====================================================================
    const std::string Tree_db::k_db_type("Tree");
    
    Tree_db::Enumerator::Enumerator(TCBDB *db,
                                    Tree_db::Enumerator::Direction dir) : dir_(dir), cur_(0), more_cache_(false)
    {
        cur_ = tcbdbcurnew(db);
        if (Tree_db::Enumerator::k_forward == dir_)
        {
            more_cache_ = tcbdbcurfirst(cur_);
        }
        else
        {
            more_cache_ = tcbdbcurlast(cur_);
        }
    }
    
    Tree_db::Enumerator::~Enumerator()
    {
        if (cur_)
        {
            tcbdbcurdel(cur_);
            cur_ = NULL;
        }
    }
    
    bool Tree_db::Enumerator::more()
    {
        bool tmp = more_cache_;
        if (!tmp)
        {
            delete this;
        }
        return tmp;
    }
    
    DB::value_t Tree_db::Enumerator::next_key()
    {
        if (!more())
        {
            throw new lj::Exception("DBerror", "Enumerator past end.");
        }
        
        int sz = 0;
        void* ptr = tcbdbcurkey(cur_, &sz);
        if (!ptr || !sz)
        {
            ptr = 0;
        }
        
        return value_t(ptr, sz);
    }
    
    DB::value_t Tree_db::Enumerator::next()
    {
        if (!more())
        {
            throw new lj::Exception("DBerror", "Enumerator past end.");
        }
        
        int sz = 0;
        void* ptr = tcbdbcurval(cur_, &sz);
        if (!ptr || !sz)
        {
            ptr = 0;
        }
        
        if (Tree_db::Enumerator::k_forward == dir_)
        {
            more_cache_ = tcbdbcurnext(cur_);
        }
        else
        {
            more_cache_ = tcbdbcurprev(cur_);
        }
        
        return value_t(ptr, sz);
    }
    
    Tree_db::Tree_db(const std::string &filename,
                     const int mode,
                     Tree_db::Tune_function_pointer db_tune_func,
                     const void *ptr)
    {
        db_ = tcbdbnew();
        if (db_tune_func)
        {
            (*db_tune_func)(db(), ptr);
        }
        tcbdbopen(db(), filename.c_str(), mode);
    }
    
    Tree_db::~Tree_db()
    {
        tcbdbclose(db_);
        tcbdbdel(db_);
        db_ = 0;
    }
    
    DB::value_t Tree_db::at(const void* key, const size_t len)
    {
        int sz = 0;
        void* ptr = tcbdbget(db(), key, len, &sz);
        if (!ptr || !sz)
        {
            ptr = 0;
        }
        return value_t(ptr, sz);
    }
    
    bool Tree_db::at_together(const void* key,
                              const size_t len,
                              list_value_t& results)
    {
        TCLIST* ptr = tcbdbget4(db(), key, len);
        if (!ptr)
        {
            return false;
        }
        
        void* item;
        int sz;
        while (tclistnum(ptr))
        {
            item = tclistshift(ptr, &sz);
            if (!item || !sz)
            {
                continue;
            }
            results.push_back(value_t(item,sz));
        }
        tclistdel(ptr);
        return true;
    }
    
    bool Tree_db::at_range(const void* start,
                           const size_t start_len,
                           const bool start_inc,
                           const void* end,
                           const size_t end_len,
                           const bool end_inc,
                           list_value_t& results)
    {
        list_value_t keys;
        if (range_keys(start,
                       start_len,
                       start_inc,
                       end,
                       end_len,
                       end_inc,
                       keys))
        {
            for (list_value_t::const_iterator iter = keys.begin();
                 keys.end() != iter;
                 ++iter)
            {
                at_together(iter->first, iter->second, results);
                free(iter->first);
            }
            return true;
        }
        return false;
    }
    
    void Tree_db::place(const void* key,
                        const size_t key_len,
                        const void* const val, 
                        const size_t val_len)
    {
        if (!tcbdbput(db(), key, key_len, val, val_len))
        {
            throw new Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::place_with_existing(const void* key,
                                      const size_t key_len,
                                      const void* const val,
                                      const size_t val_len)
    {
        if (!tcbdbputdup(db(), key, key_len, val, val_len))
        {
            throw new Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::place_together(const void* key,
                                 const size_t key_len,
                                 const list_value_t& vals)
    {
        // XXX This should probably be modified to build a TCLIST, then call
        // a different method.
        for (list_value_t::const_iterator val = vals.begin();
             vals.end() != val;
             ++val)
        {
            place_with_existing(key, key_len, val->first, val->second);
        }
    }
    
    void Tree_db::place_if_absent(const void* key, 
                                  const size_t key_len, 
                                  const void* const val,
                                  const size_t val_len)
    {
        if (!tcbdbputkeep(db(), key, key_len, val, val_len))
        {
            throw new Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::place_or_append(const void* key, 
                                  const size_t key_len, 
                                  const void* const val,
                                  const size_t val_len)
    {
        if (!tcbdbputcat(db(), key, key_len, val, val_len))
        {
            throw new Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::remove(const void* key, const size_t len)
    {
        if (!tcbdbout(db(), key, len))
        {
            throw new Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::remove_together(const void* key, const size_t len)
    {
        if(!tcbdbout3(db(), key, len))
        {
            throw new Exception("DBerror", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::remove_from_existing(const void* key,
                                       const size_t len,
                                       const void* const val,
                                       const size_t val_len)
    {
        list_value_t values;
        at_together(key, len, values);
        
        bool rewrite = false;
        for (list_value_t::iterator iter = values.begin();
             values.end() != iter;
             ++iter)
        {
            if (arrays_equal(val, val_len, iter->first, iter->second))
            {
                free(iter->first);
                values.erase(iter);
                rewrite = true;
                break;
            }
        }
        
        if (rewrite)
        {
            remove_together(key, len);
            place_together(key, len, values);
        }
        
        for (list_value_t::const_iterator iter = values.begin();
             values.end() != iter;
             ++iter)
        {
            free(iter->first);
        }
    }
    
    void Tree_db::start_writes()
    {
        if (!tcbdbtranbegin(db()))
        {
            throw new Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::save_writes()
    {
        if (!tcbdbtrancommit(db()))
        {
            throw new Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::abort_writes()
    {
        if (!tcbdbtranabort(db()))
        {
            throw new Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    long long Tree_db::count()
    {
        return (long long)tcbdbrnum(db());
    }
    
    DB::value_t Tree_db::max_key()
    {
        BDBCUR* cur = tcbdbcurnew(db());
        if (!tcbdbcurlast(cur))
        {
            tcbdbcurdel(cur);
            unsigned long long* ptr = (unsigned long long*)malloc(sizeof(unsigned long long));
            *ptr = 0;
            return value_t(ptr, sizeof(unsigned long long));
        }
        
        int sz = 0;
        void* ptr = tcbdbcurkey(cur, &sz);
        tcbdbcurdel(cur);
        if (!ptr || !sz)
        {
            throw new Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
        }
        
        return value_t(ptr, sz);
    }
    
    DB::value_t Tree_db::min_key()
    {
        BDBCUR* cur = tcbdbcurnew(db());
        if (!tcbdbcurfirst(cur))
        {
            tcbdbcurdel(cur);
            unsigned long long*ptr = (unsigned long long*)malloc(sizeof(unsigned long long));
            *ptr = 0;
            return value_t(ptr, sizeof(unsigned long long));
        }
        
        int sz = 0;
        void* ptr = tcbdbcurkey(cur, &sz);
        tcbdbcurdel(cur);
        
        if(!ptr || !sz)
        {
            throw new Exception("DB error", tcbdberrmsg(tcbdbecode(db())));
        }
        
        return value_t(ptr, sz);
    }
    
    bool Tree_db::range_keys(const void* start,
                             const size_t start_len,
                             const bool start_inc,
                             const void* end,
                             const size_t end_len,
                             const bool end_inc,
                             list_value_t& keys)
    {
        TCLIST* ptr = tcbdbrange(db(),
                                 start,
                                 start_len,
                                 start_inc,
                                 end,
                                 end_len,
                                 end_inc,
                                 -1);
        if (!ptr)
        {
            return false;
        }
        
        void *key;
        int key_sz;
        while (tclistnum(ptr))
        {
            key = tclistshift(ptr, &key_sz);
            if (!key || !key_sz)
            {
                continue;
            }
            keys.push_back(value_t(key, key_sz));
        }
        tclistdel(ptr);
        return true;
    }
    
    Tree_db::Enumerator* Tree_db::forward_enumerator()
    {
        return new Tree_db::Enumerator(db(), Tree_db::Enumerator::k_forward);
    }
    
    Tree_db::Enumerator* Tree_db::backward_enumerator()
    {
        return new Tree_db::Enumerator(db(), Tree_db::Enumerator::k_backward);
    }
    
    void Tree_db::truncate()
    {
        if (!tcbdbvanish(db()))
        {
            throw new Exception("TokyoTree", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::copy(const std::string &target)
    {
        if (!tcbdbcopy(db(), target.c_str()))
        {
            throw new Exception("TokyoTree", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    void Tree_db::optimize()
    {
        if (!tcbdboptimize(db(), -1, -1, -1, -1, -1, 0xFF))
        {
            throw new Exception("TokyoTreeOptimize", tcbdberrmsg(tcbdbecode(db())));
        }
    }
    
    //=====================================================================
    // Fixed_db Implementation
    //=====================================================================
    const std::string Fixed_db::k_db_type("Fixed");
    
    Fixed_db::Enumerator::Enumerator(TCFDB *db) : last_(0), more_cache_(true), db_(db)
    {
        tcfdbiterinit(db_);
        last_ = tcfdbiternext(db_);
        more_cache_ = (0 < last_);
    }
    
    Fixed_db::Enumerator::~Enumerator()
    {
    }
    
    bool Fixed_db::Enumerator::more()
    {
        bool tmp = more_cache_;
        if (!tmp)
        {
            delete this;
        }
        return tmp;
    }
    
    uint64_t Fixed_db::Enumerator::next_key()
    {
        if (!more())
        {
            throw new lj::Exception("TokyoFixedEnumeratorNextKey", "Enumerator past end.");
        }
        
        return last_;
    }
    
    DB::value_t Fixed_db::Enumerator::next()
    {
        if (!more())
        {
            throw new lj::Exception("TokyoFixedEnumeratorNext", "Enumerator past end.");
        }
        
        int sz = 0;
        void* ptr = tcfdbget(db_, last_, &sz);
        if (!ptr || !sz)
        {
            ptr = 0;
        }
        
        last_ = tcfdbiternext(db_);
        more_cache_ = (0 < last_);
        
        return value_t(ptr, sz);
    }
    
    Fixed_db::Fixed_db(const std::string& filename,
                     const int mode,
                     Fixed_db::Tune_function_pointer db_tune_func,
                     const void* ptr)
    {
        db_ = tcfdbnew();
        if (db_tune_func)
        {
            (*db_tune_func)(db(), ptr);
        }
        tcfdbopen(db(), filename.c_str(), mode);
    }
    
    Fixed_db::~Fixed_db()
    {
        tcfdbclose(db_);
        tcfdbdel(db_);
        db_ = 0;
    }
    
    DB::value_t Fixed_db::at(const void* key, const size_t len)
    {
        int64_t real_key = *static_cast<const int64_t*>(key);
        
        int sz = 0;
        void* ptr = tcfdbget(db(), real_key, &sz);
        if (!ptr || !sz)
        {
            ptr = 0;
        }
        return value_t(ptr, sz);
    }
    
    void Fixed_db::place(const void* key,
                        const size_t key_len,
                        const void* const val, 
                        const size_t val_len)
    {
        int64_t real_key = *static_cast<const int64_t*>(key);
        
        if (!tcfdbput(db(), real_key, val, val_len))
        {
            throw new Exception("TokyoFixedPlace", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::place_if_absent(const void* key, 
                                  const size_t key_len, 
                                  const void* const val,
                                  const size_t val_len)
    {
        int64_t real_key = *static_cast<const int64_t*>(key);
        
        if (!tcfdbputkeep(db(), real_key, val, val_len))
        {
            throw new Exception("TokyoFixedPlaceIfAbsent", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::place_or_append(const void*key, 
                                  const size_t key_len, 
                                  const void* const val,
                                  const size_t val_len)
    {
        int64_t real_key = *static_cast<const int64_t*>(key);
        
        if (!tcfdbputcat(db(), real_key, val, val_len))
        {
            throw new Exception("TokyoFixedPlaceOrAddpend", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::remove(const void* key, const size_t len)
    {
        int64_t real_key = *static_cast<const int64_t*>(key);
        
        if (!tcfdbout(db(), real_key))
        {
            throw new Exception("TokyoFixedRemove", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::start_writes()
    {
        if (!tcfdbtranbegin(db()))
        {
            throw new Exception("TokyoFixedStartWrites", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::save_writes()
    {
        if (!tcfdbtrancommit(db()))
        {
            throw new Exception("TokyoFixedSaveWrites", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::abort_writes()
    {
        if (!tcfdbtranabort(db()))
        {
            throw new Exception("TokyoFixedAbortWrites", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    long long Fixed_db::count()
    {
        return (long long)tcfdbrnum(db());
    }
    
    void Fixed_db::truncate()
    {
        if (!tcfdbvanish(db()))
        {
            throw new Exception("TokyoFixedVanish", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::optimize()
    {
        if (!tcfdboptimize(db(), -1, -1))
        {
            throw new Exception("TokyoFixedOptimize", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    void Fixed_db::copy(const std::string &target)
    {
        if (!tcfdbcopy(db(), target.c_str()))
        {
            throw new Exception("TokyoFixedCopy", tcfdberrmsg(tcfdbecode(db())));
        }
    }
    
    Fixed_db::Enumerator* Fixed_db::enumerator()
    {
        return new Fixed_db::Enumerator(db());
    }
};
