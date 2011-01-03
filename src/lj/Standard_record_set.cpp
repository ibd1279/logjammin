/*!
 \file Standard_record_set.cpp
 \brief LJ Standard_record_set implementation.
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

#include "lj/Standard_record_set.h"

#include "tokyo/Tokyo.h"
#include "lj/Logger.h"
#include "lj/Storage.h"

namespace lj
{
    Standard_record_set::Standard_record_set(const Storage* storage,
                                             const std::set<unsigned long long>& keys,
                                             const Record_set::Operation op) : storage_(storage), keys_(0), op_(op), raw_size_(0)
    {
        keys_ = new std::set<unsigned long long>(keys);
    }
    
    Standard_record_set::Standard_record_set(const Storage* storage,
                                             std::set<unsigned long long>* keys,
                                             const Record_set::Operation op) : storage_(storage), keys_(keys), op_(op)
    {
    }
    
    Standard_record_set::Standard_record_set(const Standard_record_set& orig) : storage_(orig.storage_), keys_(0), op_(orig.op_)
    {
        keys_ = new std::set<unsigned long long>(*orig.keys_);
    }
    
    Standard_record_set::~Standard_record_set()
    {
        storage_ = NULL;
        if (keys_)
        {
            delete keys_;
        }
    }
    
    Record_set& Standard_record_set::set_operation(const Record_set::Operation op)
    {
        op_ = op;
        return *this;
    }
    
    bool Standard_record_set::is_included(const unsigned long long key) const
    {
        return keys_->end() != keys_->find(key);
    }
    
    std::unique_ptr<Record_set> Standard_record_set::include_keys(const std::set<unsigned long long>& keys)
    {
        Standard_record_set* ptr = new Standard_record_set(*this);
        ptr->keys_->insert(keys.begin(), keys.end());
        ptr->set_raw_size(size());
        return std::unique_ptr<Record_set>(ptr);
    }
    
    std::unique_ptr<Record_set> Standard_record_set::include_key(const unsigned long long key)
    {
        Standard_record_set* ptr = new Standard_record_set(*this);
        ptr->keys_->insert(key);
        ptr->set_raw_size(size());
        return std::unique_ptr<Record_set>(ptr);
    }
    
    std::unique_ptr<Record_set> Standard_record_set::exclude_keys(const std::set<unsigned long long>& keys)
    {
        Standard_record_set* ptr = new Standard_record_set(*this);
        for (std::set<unsigned long long>::const_iterator iter = keys.begin();
             keys.end() != iter;
             ++iter)
        {
            ptr->keys_->erase(*iter);
        }
        ptr->set_raw_size(size());
        return std::unique_ptr<Record_set>(ptr);
    }
    
    std::unique_ptr<Record_set> Standard_record_set::exclude_key(const unsigned long long key)
    {
        Standard_record_set* ptr = new Standard_record_set(*this);
        ptr->keys_->erase(key);
        return std::unique_ptr<Record_set>(ptr);
    }
    
    std::unique_ptr<Record_set> Standard_record_set::equal(const std::string& indx,
                                                         const void* const val,
                                                         const size_t len) const
    {
        Log::debug.log("Equal on [%s] with [%d][%s].") << indx << len << ((char *)val) << Log::end;
        tokyo::Hash_db* hash_index = Record_set::storage_hash(storage_, indx);
        tokyo::Tree_db* tree_index = Record_set::storage_tree(storage_, indx);
        
        tokyo::DB::list_value_t db_values;
        if (hash_index)
        {
            db_values.push_back(hash_index->at(val, len));
        }
        else if (tree_index)
        {
            tree_index->at_together(val, len, db_values);
        }
        else
        {
            return std::unique_ptr<Record_set>(new Standard_record_set(*this));
        }
        
        std::set<unsigned long long> storage_keys;
        Record_set::list_to_set(db_values, storage_keys);
        std::set<unsigned long long>* output = Record_set::operate_on_sets<std::set<unsigned long long> >(op_, *keys_, storage_keys);
        Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
        
        std::unique_ptr<Record_set> ptr(new Standard_record_set(storage_, output, op_));
        ptr->set_raw_size(storage_keys.size());
        return ptr;
    }
    
    std::unique_ptr<Record_set> Standard_record_set::greater(const std::string& indx,
                                                           const void* const val,
                                                           const size_t len) const
    {
        Log::debug.log("Greater on [%s] with [%d][%s].") << indx << len << ((char *)val) << Log::end;
        tokyo::Tree_db* tree_index = Record_set::storage_tree(storage_, indx);
        
        tokyo::DB::list_value_t db_values;
        if (tree_index)
        {
            tokyo::DB::value_t max = tree_index->max_key();
            tree_index->at_range(val,
                                 len,
                                 false,
                                 max.first,
                                 max.second,
                                 true,
                                 db_values);
        }
        else
        {
            return std::unique_ptr<Record_set>(new Standard_record_set(*this));
        }
        
        std::set<unsigned long long> storage_keys;
        Record_set::list_to_set(db_values, storage_keys);
        std::set<unsigned long long>* output = Record_set::operate_on_sets<std::set<unsigned long long> >(op_, *keys_, storage_keys);
        Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
        
        std::unique_ptr<Record_set> ptr(new Standard_record_set(storage_, output, op_));
        ptr->set_raw_size(storage_keys.size());
        return ptr;
    }    
    
    std::unique_ptr<Record_set> Standard_record_set::lesser(const std::string& indx,
                                                          const void* const val,
                                                          const size_t len) const
    {
        Log::debug.log("Lesser on [%s] with [%d][%s].") << indx << len << ((char *)val) << Log::end;
        tokyo::Tree_db* tree_index = Record_set::storage_tree(storage_, indx);
        
        tokyo::DB::list_value_t db_values;
        if (tree_index)
        {
            tokyo::DB::value_t min = tree_index->min_key();
            tree_index->at_range(min.first,
                                 min.second,
                                 true,
                                 val,
                                 len,
                                 false,
                                 db_values);
        }
        else
        {
            return std::unique_ptr<Record_set>(new Standard_record_set(*this));
        }
        
        std::set<unsigned long long> storage_keys;
        Record_set::list_to_set(db_values, storage_keys);
        std::set<unsigned long long>* output = Record_set::operate_on_sets<std::set<unsigned long long> >(op_, *keys_, storage_keys);
        Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
        
        std::unique_ptr<Record_set> ptr(new Standard_record_set(storage_, output, op_));
        ptr->set_raw_size(storage_keys.size());
        return ptr;
    }
    
    
    long long Standard_record_set::size() const
    {
        return keys_->size();
    }
    
    bool Standard_record_set::items(std::list<Bson>& records) const
    {
        bool modified = false;
        for (std::set<unsigned long long>::const_iterator iter = keys_->begin();
             keys_->end() != iter;
             ++iter)
        {
            modified = true;
            records.push_back(*doc_at(*iter, true));
        }
        return modified;
    }
    
    bool Standard_record_set::items(std::list<Bson*>& records) const
    {
        bool modified = false;
        for (std::set<unsigned long long>::const_iterator iter = keys_->begin();
             keys_->end() != iter;
             ++iter)
        {
            modified = true;
            records.push_back(doc_at(*iter, true).release());
        }
        return modified;
    }
    
    bool Standard_record_set::first(Bson& result) const
    {
        for (std::set<unsigned long long>::const_iterator iter = keys_->begin();
             keys_->end() != iter;
             ++iter)
        {
            result.copy_from(*doc_at(*iter, true));
            return true;
        }
        return false;
    }
    
    bool Standard_record_set::items_raw(lj::Bson& records) const
    {
        bool modified = false;
        for (std::set<unsigned long long>::const_iterator iter = keys_->begin();
             keys_->end() != iter;
             ++iter)
        {
            modified = true;
            records.push_child("", doc_at(*iter, false).release());
        }
        return modified;
    }
    
    void Standard_record_set::set_raw_size(long long sz)
    {
        raw_size_ = sz;
    }
    
    long long Standard_record_set::raw_size() const
    {
        return raw_size_;
    }
    
    const lj::Storage& Standard_record_set::storage() const
    {
        return *storage_;
    }
    
    std::unique_ptr<Bson> Standard_record_set::doc_at(unsigned long long pkey,
                                                    bool marshall) const
    {
        tokyo::Tree_db* db = Record_set::storage_db(storage_);
        tokyo::DB::value_t p = db->at(&pkey, sizeof(unsigned long long));
        if (!p.first)
        {
            return std::unique_ptr<Bson>(new Bson());
        }
        Bson* ptr = new Bson(marshall ? Bson::k_document : Bson::k_binary_document,
                             static_cast<char *>(p.first));
        free(p.first);
        return std::unique_ptr<Bson>(ptr);
    }
}; // namespace lj
