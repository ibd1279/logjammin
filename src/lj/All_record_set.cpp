/*!
 \file All_record_set.cpp
 \brief LJ All_record_set implementation.
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

#include "All_record_set.h"

#include "lj/Logger.h"
#include "lj/Standard_record_set.h"
#include "lj/Storage.h"
#include "tokyo/Tokyo.h"

namespace lj
{
    All_record_set::All_record_set(const Storage* storage,
                                   const Record_set::Operation op) : storage_(storage), op_(op)
    {
    }
    
    All_record_set::All_record_set(const All_record_set& orig) : storage_(orig.storage_), op_(orig.op_)
    {
    }
    
    All_record_set::~All_record_set()
    {
        storage_ = 0;
    }
    
    Record_set& All_record_set::set_operation(const Record_set::Operation op)
    {
        op_ = op;
        return *this;
    }
    
    bool All_record_set::is_included(const unsigned long long key) const
    {
        return true;
    }
    
    std::auto_ptr<Record_set> All_record_set::include_keys(const std::set<unsigned long long>& keys)
    {
        return std::auto_ptr<Record_set>(new All_record_set(*this));
    }
    
    std::auto_ptr<Record_set> All_record_set::include_key(const unsigned long long key)
    {
        return std::auto_ptr<Record_set>(new All_record_set(*this));
    }
    
    std::auto_ptr<Record_set> All_record_set::exclude_keys(const std::set<unsigned long long> &keys)
    {
        std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
        get_all_keys(real_keys);
        std::auto_ptr<Record_set> ptr(new Standard_record_set(storage_,
                                                              real_keys,
                                                              op_));
        return ptr->exclude_keys(keys);
    }
    
    std::auto_ptr<Record_set> All_record_set::exclude_key(const unsigned long long key)
    {
        std::set<unsigned long long> tmp;
        tmp.insert(key);
        return exclude_keys(tmp);
    }
    
    std::auto_ptr<Record_set> All_record_set::equal(const std::string& indx,
                                                    const void* const val,
                                                    const size_t len) const
    {
        std::auto_ptr<Record_set> ptr;
        if (Record_set::k_union == op_)
        {
            ptr.reset(new All_record_set(*this));
        }
        else if (Record_set::k_intersection == op_)
        {
            Standard_record_set tmp(storage_,
                                    new std::set<unsigned long long>(),
                                    Record_set::k_union);
            ptr = tmp.equal(indx, val, len);
            ptr->set_operation(op_);
        }
        else
        {
            std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
            get_all_keys(real_keys);
            Standard_record_set tmp(storage_, real_keys, op_);
            ptr = tmp.equal(indx, val, len);
        }
        
        return ptr;
    }
    
    std::auto_ptr<Record_set> All_record_set::greater(const std::string& indx,
                                                      const void* const val,
                                                      const size_t len) const
    {
        std::auto_ptr<Record_set> ptr;
        if (Record_set::k_union == op_)
        {
            ptr.reset(new All_record_set(*this));
        }
        else if (Record_set::k_intersection == op_)
        {
            Standard_record_set tmp(storage_,
                                    new std::set<unsigned long long>(),
                                    Record_set::k_union);
            ptr = tmp.greater(indx, val, len);
            ptr->set_operation(op_);
        }
        else
        {
            std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
            get_all_keys(real_keys);
            Standard_record_set tmp(storage_, real_keys, op_);
            ptr = tmp.greater(indx, val, len);
        }
        
        return ptr;
    }
    
    std::auto_ptr<Record_set> All_record_set::lesser(const std::string& indx,
                                                     const void* const val,
                                                     const size_t len) const
    {
        std::auto_ptr<Record_set> ptr;
        if (Record_set::k_union == op_)
        {
            ptr.reset(new All_record_set(*this));
        }
        else if (Record_set::k_intersection == op_)
        {
            Standard_record_set tmp(storage_,
                                    new std::set<unsigned long long>(),
                                    Record_set::k_union);
            ptr = tmp.lesser(indx, val, len);
            ptr->set_operation(op_);
        }
        else
        {
            std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
            get_all_keys(real_keys);
            Standard_record_set tmp(storage_, real_keys, op_);
            ptr = tmp.lesser(indx, val, len);
        }
        
        return ptr;
    }
    
    std::auto_ptr<Record_set> All_record_set::contains(const std::string& indx,
                                                       const std::string& term) const
    {
        std::auto_ptr<Record_set> ptr;
        if (Record_set::k_union == op_)
        {
            ptr.reset(new All_record_set(*this));
        }
        else if (Record_set::k_intersection == op_)
        {
            Standard_record_set tmp(storage_,
                                    new std::set<unsigned long long>(),
                                    Record_set::k_union);
            ptr = tmp.contains(indx, term);
            ptr->set_operation(op_);
        }
        else
        {
            std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
            get_all_keys(real_keys);
            Standard_record_set tmp(storage_, real_keys, op_);
            ptr = tmp.contains(indx, term);
        }
        
        return ptr;
    }
    
    std::auto_ptr<Record_set> All_record_set::tagged(const std::string& indx,
                                                     const std::string& word) const
    {
        std::auto_ptr<Record_set> ptr;
        if (Record_set::k_union == op_)
        {
            ptr.reset(new All_record_set(*this));
        }
        else if (Record_set::k_intersection == op_)
        {
            Standard_record_set tmp(storage_,
                                    new std::set<unsigned long long>(),
                                    Record_set::k_union);
            ptr = tmp.tagged(indx, word);
            ptr->set_operation(op_);
        }
        else
        {
            std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
            get_all_keys(real_keys);
            Standard_record_set tmp(storage_, real_keys, op_);
            ptr = tmp.tagged(indx, word);
        }
        
        return ptr;
    }
    
    long long All_record_set::size() const
    {
        return Record_set::storage_db(storage_)->count();
    }
    
    bool All_record_set::items(std::list<Bson>& records) const
    {
        tokyo::Tree_db* db = Record_set::storage_db(storage_);
        tokyo::Tree_db::Enumerator* e = db->forward_enumerator();
        bool modified = false;
        while (e->more())
        {
            tokyo::DB::value_t p = e->next();
            if (!p.first)
            {
                continue;
            }
            modified = true;
            Bson n(Bson::k_document, static_cast<char *>(p.first));
            records.push_back(n);
            free(p.first);
        }
        return modified;
    }
    
    bool All_record_set::items(std::list<Bson*>& records) const
    {
        tokyo::Tree_db* db = Record_set::storage_db(storage_);
        tokyo::Tree_db::Enumerator* e = db->forward_enumerator();
        bool modified = false;
        while (e->more())
        {
            tokyo::DB::value_t p = e->next();
            if (!p.first)
            {
                continue;
            }
            modified = true;
            Bson* n = new Bson(Bson::k_document, static_cast<char *>(p.first));
            records.push_back(n);
            free(p.first);
        }
        return modified;
    }
    
    bool All_record_set::first(Bson& result) const
    {
        tokyo::Tree_db* db = Record_set::storage_db(storage_);
        tokyo::Tree_db::Enumerator* e = db->forward_enumerator();
        if (e->more())
        {
            tokyo::DB::value_t p = e->next();
            if (!p.first)
            {
                return false;
            }
            result.set_value(Bson::k_document, static_cast<char *>(p.first));
            free(p.first);
            delete e;
        }
        return true;
    }
    
    bool All_record_set::items_raw(lj::Bson& records) const
    {
        tokyo::Tree_db* db = Record_set::storage_db(storage_);
        tokyo::Tree_db::Enumerator* e = db->forward_enumerator();
        bool modified = false;
        while (e->more())
        {
            tokyo::DB::value_t p = e->next();
            if (!p.first)
            {
                continue;
            }
            modified = true;
            Bson* n = new Bson(Bson::k_binary_document, static_cast<char *>(p.first));
            records.push_child("", n);
            free(p.first);
        }
        return modified;
    }

    void All_record_set::set_raw_size(long long sz)
    {
    }
    
    long long All_record_set::raw_size() const
    {
        return Record_set::storage_db(storage_)->count();
    }
    
    const lj::Storage& All_record_set::storage() const
    {
        return *storage_;
    }
    
    void All_record_set::get_all_keys(std::set<unsigned long long>* result_keys) const
    {
        tokyo::Tree_db* db = Record_set::storage_db(storage_);
        tokyo::DB::list_value_t keys;
        tokyo::DB::value_t max = db->max_key();
        tokyo::DB::value_t min = db->min_key();
        if (db->range_keys(min.first,
                           min.second,
                           true,
                           max.first,
                           max.second,
                           true,
                           keys))
        {
            Record_set::list_to_set(keys, *result_keys);
        }                    
    }
}; // namespace lj