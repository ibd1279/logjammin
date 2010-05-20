/*!
 \file Storage.cpp
 \brief LJ Storage implementation.
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

#include "lj/Storage.h"

#include "build/default/config.h"
#include "lj/Exception.h"
#include "lj/Logger.h"

using tokyo::TextSearcher;
using tokyo::TagSearcher;

namespace lj
{
    namespace {
        void dbvalue_to_storagekey(const tokyo::DB::list_value_t &ptr,
                                   std::set<unsigned long long> &keys) {
            for (tokyo::DB::list_value_t::const_iterator iter = ptr.begin();
                 ptr.end() != iter;
                 ++iter)
            {
                unsigned long long *x = static_cast<unsigned long long *>(iter->first);
                if (x)
                {
                    keys.insert(*x);
                    free(iter->first);
                }
            }
        }
        
        //! Perform an operation on two sets.
        /*!
         \par
         The provided \c op is performed on the two sets and the result is returned
         as a pointer to a third set. The operation is performed such that \c a
         \c op \c b . So keep in mind the commutative properties of the operation
         you are performing.
         \param op The operation to perform.
         \param a The first set.
         \param b The second set.
         \return A newly created set.
         */
        template<typename T>
        T *operate_on_sets(const Record_set::Operation op,
                           const T& a,
                           const T& b)
        {
            const T* small = (a.size() < b.size()) ? &a : &b;
            const T* big = (a.size() < b.size()) ? &b : &a;
            T* rs = new T();
            typename T::iterator inserted_at = rs->begin();
            switch (op)
            {
                case Record_set::k_intersection:
                    for (typename T::const_iterator iter = small->begin();
                         small->end() != iter;
                         ++iter)
                    {
                        if (big->end() != big->find(*iter))
                        {
                            inserted_at = rs->insert(inserted_at, *iter);
                        }
                    }
                    break;
                case Record_set::k_union:
                    rs->insert(big->begin(), big->end());
                    rs->insert(small->begin(), small->end());
                    break;
                case Record_set::k_symmetric_difference:
                    for (typename T::const_iterator iter = b.begin();
                         b.end() != iter;
                         ++iter)
                    {
                        if (a.end() == a.find(*iter))
                        {
                            inserted_at = rs->insert(inserted_at, *iter);
                        }
                    }
                    inserted_at = rs->begin();
                    // fall through.
                case Record_set::k_complement:
                    for (typename T::const_iterator iter = a.begin();
                         a.end() != iter;
                         ++iter)
                    {
                        if (b.end() == b.find(*iter))
                        {
                            inserted_at = rs->insert(inserted_at, *iter);
                        }
                    }
                    break;
            }
            return rs;
        }
        
        //=====================================================================
        // Record_set Implementation.
        //=====================================================================
        
        class Standard_record_set : public Record_set {
        public:
            Standard_record_set(const Storage* storage,
                                const std::set<unsigned long long>& keys,
                                const Record_set::Operation op) : storage_(storage), keys_(0), op_(op)
            {
                keys_ = new std::set<unsigned long long>(keys);
            }
            
            Standard_record_set(const Storage* storage,
                                std::set<unsigned long long>* keys,
                                const Record_set::Operation op) : storage_(storage), keys_(keys), op_(op)
            {
            }
            
            Standard_record_set(const Standard_record_set& orig) : storage_(orig.storage_), keys_(0), op_(orig.op_)
            {
                keys_ = new std::set<unsigned long long>(*orig.keys_);
            }
            
            virtual ~Standard_record_set()
            {
                storage_ = NULL;
                if (keys_)
                {
                    delete keys_;
                }
            }
            
            virtual Record_set& set_operation(const Record_set::Operation op)
            {
                op_ = op;
                return *this;
            }
            
            virtual bool is_included(const unsigned long long key) const
            {
                return keys_->end() != keys_->find(key);
            }
            
            virtual std::auto_ptr<Record_set> include_keys(const std::set<unsigned long long>& keys)
            {
                Standard_record_set* ptr = new Standard_record_set(*this);
                ptr->keys_->insert(keys.begin(), keys.end());
                return std::auto_ptr<Record_set>(ptr);
            }
            
            virtual std::auto_ptr<Record_set> include_key(const unsigned long long key)
            {
                Standard_record_set* ptr = new Standard_record_set(*this);
                ptr->keys_->insert(key);
                return std::auto_ptr<Record_set>(ptr);
            }
            
            virtual std::auto_ptr<Record_set> exclude_keys(const std::set<unsigned long long> &keys)
            {
                Standard_record_set* ptr = new Standard_record_set(*this);
                for (std::set<unsigned long long>::const_iterator iter = keys.begin();
                     keys.end() != iter;
                     ++iter)
                {
                    ptr->keys_->erase(*iter);
                }
                return std::auto_ptr<Record_set>(ptr);
            }
            
            virtual std::auto_ptr<Record_set> exclude_key(const unsigned long long key)
            {
                Standard_record_set* ptr = new Standard_record_set(*this);
                ptr->keys_->erase(key);
                return std::auto_ptr<Record_set>(ptr);
            }
            
            virtual std::auto_ptr<Record_set> equal(const std::string& indx,
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
                    return std::auto_ptr<Record_set>(new Standard_record_set(*this));
                }
                
                std::set<unsigned long long> storage_keys;
                dbvalue_to_storagekey(db_values, storage_keys);
                std::set<unsigned long long>* output = operate_on_sets<std::set<unsigned long long> >(op_, *keys_, storage_keys);
                Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
                return std::auto_ptr<Record_set>(new Standard_record_set(storage_, output, op_));
            }
            
            virtual std::auto_ptr<Record_set> greater(const std::string& indx,
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
                    return std::auto_ptr<Record_set>(new Standard_record_set(*this));
                }
                
                std::set<unsigned long long> storage_keys;
                dbvalue_to_storagekey(db_values, storage_keys);
                std::set<unsigned long long>* output = operate_on_sets<std::set<unsigned long long> >(op_, *keys_, storage_keys);
                Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
                return std::auto_ptr<Record_set>(new Standard_record_set(storage_, output, op_));
            }    
            
            virtual std::auto_ptr<Record_set> lesser(const std::string& indx,
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
                    return std::auto_ptr<Record_set>(new Standard_record_set(*this));
                }
                
                std::set<unsigned long long> storage_keys;
                dbvalue_to_storagekey(db_values, storage_keys);
                std::set<unsigned long long>* output = operate_on_sets<std::set<unsigned long long> >(op_, *keys_, storage_keys);
                Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
                return std::auto_ptr<Record_set>(new Standard_record_set(storage_, output, op_));
            }
            
            virtual std::auto_ptr<Record_set> contains(const std::string& indx,
                                                       const std::string& term) const
            {
                Log::debug.log("Contains on [%s] with [%s]") << indx << term << Log::end;
                TextSearcher* text_index = Record_set::storage_text(storage_, indx);
                
                tokyo::Searcher::set_key_t searcher_values;
                if (text_index)
                {
                    text_index->search(term, searcher_values);
                }
                else
                {
                    return std::auto_ptr<Record_set>(new Standard_record_set(*this));
                }
                
                std::set<unsigned long long>* output = operate_on_sets<std::set<unsigned long long> >(op_, *keys_, searcher_values);
                Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
                return std::auto_ptr<Record_set>(new Standard_record_set(storage_, output, op_));
            }
            
            virtual std::auto_ptr<Record_set> tagged(const std::string& indx,
                                                     const std::string& word) const
            {
                Log::debug.log("Tagged on [%s] with [%s]") << indx << word << Log::end;
                TagSearcher* tag_index = Record_set::storage_tag(storage_, indx);
                
                tokyo::Searcher::set_key_t searcher_values;
                if (tag_index)
                {
                    tag_index->search(word, searcher_values);
                }
                else
                {
                    return std::auto_ptr<Record_set>(new Standard_record_set(*this));
                }
                
                std::set<unsigned long long>* output = operate_on_sets<std::set<unsigned long long> >(op_, *keys_, searcher_values);
                Log::debug.log("  %d Result%s") << output->size() << (output->size() ? "s" : "") << Log::end;
                return std::auto_ptr<Record_set>(new Standard_record_set(storage_, output, op_));
            }
            
            virtual unsigned long long size() const
            {
                return keys_->size();
            }
            
            virtual bool items(std::list<Bson>& records) const
            {
                for (std::set<unsigned long long>::const_iterator iter = keys_->begin();
                     keys_->end() != iter;
                     ++iter)
                {
                    records.push_back(doc_at(*iter));
                }
                return size();
            }
            
            virtual bool items(std::list<Bson*>& records) const
            {
                for (std::set<unsigned long long>::const_iterator iter = keys_->begin();
                     keys_->end() != iter;
                     ++iter)
                {
                    records.push_back(new Bson(doc_at(*iter)));
                }
                return size();
            }
            
            virtual bool first(Bson& result) const
            {
                for (std::set<unsigned long long>::const_iterator iter = keys_->begin();
                     keys_->end() != iter;
                     ++iter)
                {
                    result.copy_from(doc_at(*iter));
                    return true;
                }
                return false;
            }
        private:
            const Storage *storage_;
            std::set<unsigned long long>* keys_;
            Record_set::Operation op_;
            inline Bson doc_at(unsigned long long pkey) const
            {
                tokyo::Tree_db* db = Record_set::storage_db(storage_);
                tokyo::DB::value_t p = db->at(&pkey, sizeof(unsigned long long));
                if (!p.first)
                {
                    return Bson();
                }
                Bson n(Bson::k_document, static_cast<char *>(p.first));
                free(p.first);
                return n;
            }
            
            Record_set& operator=(const Standard_record_set& o);
        };
        
        class All_record_set : public Record_set {
        public:
            All_record_set(const Storage* storage,
                           const Record_set::Operation op) : storage_(storage), op_(op)
            {
            }
            
            All_record_set(const All_record_set& orig) : storage_(orig.storage_), op_(orig.op_)
            {
            }
            
            virtual ~All_record_set()
            {
                storage_ = 0;
            }
            
            virtual Record_set& set_operation(const Record_set::Operation op)
            {
                op_ = op;
                return *this;
            }
            
            virtual bool is_included(const unsigned long long key) const
            {
                return true;
            }
            
            virtual std::auto_ptr<Record_set> include_keys(const std::set<unsigned long long>& keys)
            {
                return std::auto_ptr<Record_set>(new All_record_set(*this));
            }
            
            virtual std::auto_ptr<Record_set> include_key(const unsigned long long key)
            {
                return std::auto_ptr<Record_set>(new All_record_set(*this));
            }
            
            virtual std::auto_ptr<Record_set> exclude_keys(const std::set<unsigned long long> &keys)
            {
                std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                get_all_keys(real_keys);
                std::auto_ptr<Record_set> ptr(new Standard_record_set(storage_,
                                                                      real_keys,
                                                                      op_));
                ptr->exclude_keys(keys);
                return ptr;
            }
            
            virtual std::auto_ptr<Record_set> exclude_key(const unsigned long long key)
            {
                std::set<unsigned long long> tmp;
                tmp.insert(key);
                return exclude_keys(tmp);
            }
            
            virtual std::auto_ptr<Record_set> equal(const std::string& indx,
                                                    const void* const val,
                                                    const size_t len) const
            {
                if (Record_set::k_union == op_)
                {
                    return std::auto_ptr<Record_set>(new All_record_set(*this));
                }
                else if (Record_set::k_intersection == op_)
                {
                    Standard_record_set tmp(storage_,
                                            new std::set<unsigned long long>(),
                                            Record_set::k_union);
                    std::auto_ptr<Record_set> ptr = tmp.equal(indx, val, len);
                    ptr->set_operation(op_);
                    return ptr;
                }
                else
                {
                    std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                    get_all_keys(real_keys);
                    std::auto_ptr<Record_set> ptr(new Standard_record_set(storage_,
                                                                          real_keys,
                                                                          op_));
                    return ptr->equal(indx, val, len);
                }
            }
            
            virtual std::auto_ptr<Record_set> greater(const std::string& indx,
                                                      const void* const val,
                                                      const size_t len) const
            {
                if (Record_set::k_union == op_)
                {
                    return std::auto_ptr<Record_set>(new All_record_set(*this));
                }
                else if (Record_set::k_intersection == op_)
                {
                    Standard_record_set tmp(storage_,
                                            new std::set<unsigned long long>(),
                                            Record_set::k_union);
                    std::auto_ptr<Record_set> ptr = tmp.greater(indx, val, len);
                    ptr->set_operation(op_);
                    return ptr;
                }
                else
                {
                    std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                    get_all_keys(real_keys);
                    std::auto_ptr<Record_set> ptr(new Standard_record_set(storage_,
                                                                          real_keys,
                                                                          op_));
                    return ptr->greater(indx, val, len);
                }
            }
            
            virtual std::auto_ptr<Record_set> lesser(const std::string& indx,
                                                     const void* const val,
                                                     const size_t len) const
            {
                if (Record_set::k_union == op_)
                {
                    return std::auto_ptr<Record_set>(new All_record_set(*this));
                }
                else if (Record_set::k_intersection == op_)
                {
                    Standard_record_set tmp(storage_,
                                            new std::set<unsigned long long>(),
                                            Record_set::k_union);
                    std::auto_ptr<Record_set> ptr = tmp.lesser(indx, val, len);
                    ptr->set_operation(op_);
                    return ptr;
                }
                else
                {
                    std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                    get_all_keys(real_keys);
                    std::auto_ptr<Record_set> ptr(new Standard_record_set(storage_,
                                                                          real_keys,
                                                                          op_));
                    return ptr->lesser(indx, val, len);
                }
            }
            
            virtual std::auto_ptr<Record_set> contains(const std::string& indx,
                                                       const std::string& term) const
            {
                if (Record_set::k_union == op_)
                {
                    return std::auto_ptr<Record_set>(new All_record_set(*this));
                }
                else if (Record_set::k_intersection == op_)
                {
                    Standard_record_set tmp(storage_,
                                            new std::set<unsigned long long>(),
                                            Record_set::k_union);
                    std::auto_ptr<Record_set> ptr = tmp.contains(indx, term);
                    ptr->set_operation(op_);
                    return ptr;
                }
                else
                {
                    std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                    get_all_keys(real_keys);
                    std::auto_ptr<Record_set> ptr(new Standard_record_set(storage_,
                                                                          real_keys,
                                                                          op_));
                    return ptr->contains(indx, term);
                }
            }
            
            virtual std::auto_ptr<Record_set> tagged(const std::string& indx,
                                                     const std::string& word) const
            {
                if (Record_set::k_union == op_)
                {
                    return std::auto_ptr<Record_set>(new All_record_set(*this));
                }
                else if (Record_set::k_intersection == op_)
                {
                    Standard_record_set tmp(storage_,
                                            new std::set<unsigned long long>(),
                                            Record_set::k_union);
                    std::auto_ptr<Record_set> ptr = tmp.tagged(indx, word);
                    ptr->set_operation(op_);
                    return ptr;
                }
                else
                {
                    std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                    get_all_keys(real_keys);
                    std::auto_ptr<Record_set> ptr(new Standard_record_set(storage_,
                                                                          real_keys,
                                                                          op_));
                    return ptr->tagged(indx, word);
                }
            }
            
            virtual unsigned long long size() const
            {
                std::set<unsigned long long> real_keys;
                get_all_keys(&real_keys);
                return real_keys.size();
            }
            
            virtual bool items(std::list<Bson>& records) const
            {
                std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                get_all_keys(real_keys);
                Standard_record_set tmp(storage_,
                                        real_keys,
                                        op_);
                return tmp.items(records);
            }
            
            virtual bool items(std::list<Bson*>& records) const
            {
                std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                get_all_keys(real_keys);
                Standard_record_set tmp(storage_,
                                        real_keys,
                                        op_);
                return tmp.items(records);
            }
            
            virtual bool first(Bson& result) const
            {
                std::set<unsigned long long>* real_keys = new std::set<unsigned long long>();
                get_all_keys(real_keys);
                Standard_record_set tmp(storage_,
                                        real_keys,
                                        op_);
                return tmp.first(result);
            }
        private:
            const Storage *storage_;
            Record_set::Operation op_;
            Record_set& operator=(const All_record_set& o);
            void get_all_keys(std::set<unsigned long long>* result_keys) const
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
                    dbvalue_to_storagekey(keys, *result_keys);
                }                    
            }
        };
    };
    
    //=====================================================================
    // Storage Implementation.
    //=====================================================================
    
    namespace
    {
        void storage_tree_cfg(TCBDB* db, const void* ptr)
        {
            const Bson* bn = static_cast<const Bson*>(ptr);
            if (lj::bson_as_string(bn->nav("compare")).compare("lex") == 0)
            {
                tcbdbsetcmpfunc(db, tcbdbcmplexical, NULL);
                Log::info.log("Using lexical for compares") << Log::end;
            }
            else if (lj::bson_as_string(bn->nav("compare")).compare("int32") == 0)
            {
                tcbdbsetcmpfunc(db, tcbdbcmpint32, NULL);
                Log::info.log("Using int32 for compares") << Log::end;
            }
            else
            {
                tcbdbsetcmpfunc(db, tcbdbcmpint64, NULL);
                Log::info.log("Using int64 for compares") << Log::end;
            }
            
            // XXX config other things like compression type, tree hight, etc.
        }
        
        void storage_hash_cfg(TCHDB* db, const void* ptr)
        {
            //const Bson *bn = static_cast<const Bson *>(ptr);
            
            // XXX config other things like compression type.
        }
        
        void storage_text_cfg(TCQDB* db, const void* ptr)
        {
            //const Bson *bn = static_cast<const Bson *>(ptr);
            
            // XXX config other things like compression type.
        }
        
        // XXX Add configuration method for tags
        void storage_tag_cfg(TCWDB* db, const void* ptr)
        {
            //const Bson *bn = static_cast<const Bson *>(ptr);
            
            // XXX config other things like compression type.
        }
        
        template<typename T>
        void open_storage_index(const std::string& dir,
                                const Linked_map<std::string, Bson*>& cfg,
                                int open_flags,
                                typename T::Tune_function_pointer tune_function,
                                std::map<std::string, T*>& dest)
        {
            for (Linked_map<std::string, Bson*>::const_iterator iter = cfg.begin();
                 cfg.end() != iter;
                 ++iter)
            {
                if (!(*iter).second->nav("file").exists() ||
                    !(*iter).second->nav("field").exists())
                {
                    Log::error.log("Unable to open index [%s] because file or field is not set.") << (*iter).first << Log::end; 
                    continue;
                }
                std::string indexfile(dir + "/" + bson_as_string((*iter).second->nav("file")));
                T* db = new T(indexfile,
                              open_flags,
                              tune_function,
                              (*iter).second);
                std::string field = bson_as_string((*iter).second->nav("field"));
                dest.insert(std::pair<std::string, T*>(field, db));
            }
        }
        
        std::pair<int, int> bson_to_storage_delta(const lj::Bson* ptr)
        {
            if(bson_type_is_quotable(ptr->type()))
            {
                return std::pair<int, int>(4,5);
            }
            else
            {
                return std::pair<int, int>(0,0);
            }
        }
    }; // namespace
    
    Storage::Storage(const std::string &dir) : db_(NULL), fields_tree_(), fields_hash_(), fields_text_(), fields_tag_(), nested_indexing_(), directory_(DBDIR)
    {
        directory_.append("/").append(dir);
        std::string configfile(directory_ + "/config");
        
        Log::info.log("Loading configuration from [%s].") << configfile << Log::end;
        Bson* cfg = bson_load(configfile);
        Log::info.log("Loaded Settings [%s].") << lj::bson_as_pretty_string(*cfg) << Log::end;
        
        std::string dbfile(directory_ + "/" + lj::bson_as_string(cfg->nav("main/file")));
        Log::info.log("Opening database [%s].") << dbfile << Log::end;
        db_ = new tokyo::Tree_db(dbfile,
                                 BDBOREADER | BDBOWRITER | BDBOCREAT,
                                 &storage_tree_cfg,
                                 cfg->path("main"));
        
        Log::info.log("Opening tree indices under [%s].") << directory_ << Log::end;
        open_storage_index<tokyo::Tree_db>(directory_,
                                           cfg->nav("index/tree").to_map(),
                                           BDBOREADER | BDBOWRITER | BDBOCREAT,
                                           &storage_tree_cfg,
                                           fields_tree_);
        
        Log::info.log("Opening hash indices under [%s].") << directory_ << Log::end;
        open_storage_index<tokyo::Hash_db>(directory_,
                                           cfg->nav("index/hash").to_map(),
                                           HDBOREADER | HDBOWRITER | HDBOCREAT,
                                           &storage_hash_cfg,
                                           fields_hash_);
        
        Log::info.log("Opening text indices under [%s].") << directory_ << Log::end;
        open_storage_index<TextSearcher>(directory_,
                                         cfg->nav("index/text").to_map(),
                                         QDBOREADER | QDBOWRITER | QDBOCREAT,
                                         &storage_text_cfg,
                                         fields_text_);
        
        Log::info.log("Opening tag indices under [%s].") << directory_ << Log::end;
        open_storage_index<TagSearcher>(directory_,
                                        cfg->nav("index/tag").to_map(),
                                        WDBOREADER | WDBOWRITER | WDBOCREAT,
                                        &storage_tag_cfg,
                                        fields_tag_);
        
        Log::info.log("Registering nested indexing from [%s].") << directory_ << Log::end;
        Bson *nested_fields = cfg->path("main/nested");
        for (Linked_map<std::string, Bson*>::const_iterator iter = nested_fields->to_map().begin();
             nested_fields->to_map().end() != iter;
             ++iter)
        {
            Log::info.log("Adding nested field [%s].") << lj::bson_as_string(*iter->second) << Log::end;
            nested_indexing_.insert(lj::bson_as_string(*iter->second));
        }
    }
    
    Storage::~Storage()
    {
        if (fields_tag_.size())
        {
            Log::info.log("Closing tag indicies under [%s].") << directory_ << Log::end;
            for (std::map<std::string, TagSearcher *>::const_iterator iter = fields_tag_.begin();
                 fields_tag_.end() != iter;
                 ++iter)
            {
                Log::info.log("Closing tag index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        if (fields_text_.size())
        {
            Log::info.log("Closing text indicies under [%s].") << directory_ << Log::end;
            for (std::map<std::string, TextSearcher *>::const_iterator iter = fields_text_.begin();
                 fields_text_.end() != iter;
                 ++iter)
            {
                Log::info.log("Closing text index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        if (fields_hash_.size())
        {
            Log::info.log("Closing hash indicies under [%s].") << directory_ << Log::end;
            for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
                 fields_hash_.end() != iter;
                 ++iter)
            {
                Log::info.log("Closing hash index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        if (fields_tree_.size())
        {
            Log::info.log("Closing tree indicies under [%s].") << directory_ << Log::end;
            for(std::map<std::string, tokyo::Tree_db *>::const_iterator iter = fields_tree_.begin();
                iter != fields_tree_.end();
                ++iter) {
                Log::info.log("Closing tree index for field [%s].") << iter->first << Log::end;
                delete iter->second;
            }
        }
        
        Log::info.log("Closing database for [%s].") << directory_ << Log::end;
        delete db_;
    }
    
    std::auto_ptr<Record_set> Storage::at(const unsigned long long key) const
    {
        std::auto_ptr<Record_set> ptr = none();
        ptr->include_key(key);
        return ptr;
    }
    
    std::auto_ptr<Record_set> Storage::all() const
    {
        return std::auto_ptr<Record_set>(new All_record_set(this,
                                                            Record_set::k_intersection));
    }
    
    std::auto_ptr<Record_set> Storage::none() const
    {
        return std::auto_ptr<Record_set>(new Standard_record_set(this,
                                                                 new std::set<unsigned long long>(),
                                                                 Record_set::k_union));
    }
    
    Storage &Storage::place(Bson &value)
    {
        unsigned long long key = lj::bson_as_int64(value.nav("__key"));
        unsigned long long original_key = lj::bson_as_int64(value.nav("__key"));
        
        Log::debug.log("Placing [%llu] [%s]") << key << lj::bson_as_pretty_string(value) << Log::end;
        try
        {
            begin_transaction();
            if (key)
            {
                Log::debug.log("Deindexing previous record to clean house.") << Log::end;
                deindex(key);
            }
            else
            {
                Log::debug.log("New record. calculating key.") << Log::end;
                unsigned long long *ptr = static_cast<unsigned long long *>(db_->max_key().first);
                key = (*ptr) + 1;
                free(ptr);
                Log::debug.log("New key is [%d].") << key << Log::end;
            }
            
            // Enforce unique constraints.
            Log::debug.log("Unique constraint check.") << Log::end;
            for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
                 fields_hash_.end() != iter;
                 ++iter)
            {
                Bson n(value.nav(iter->first));
                if (n.exists())
                {
                    check_unique(n, iter->first, iter->second);
                }
            }
            
            Log::debug.log("Place in DB.") << Log::end;
            value.nav("__key").set_value(Bson::k_int64, reinterpret_cast<char*>(&key));
            char* bson = value.to_binary();
            db_->place(&key,
                       sizeof(unsigned long long),
                       bson,
                       value.size());
            delete[] bson;
            
            reindex(key);
            commit_transaction();
        }
        catch(Exception* ex)
        {
            value.nav("__key").set_value(Bson::k_int64, reinterpret_cast<char *>(&original_key));
            abort_transaction();
            throw ex;
        }
        return *this;
    }
    
    Storage &Storage::remove(Bson &value)
    {
        unsigned long long key = bson_as_int64(value.nav("__key"));
        
        Log::debug.log("Removing [%llu] [%s]") << key << bson_as_pretty_string(value) << Log::end;
        
        if (key)
        {
            try
            {
                begin_transaction();
                deindex(key);
                db_->remove(&key, sizeof(unsigned long long));
                commit_transaction();
                value.nav("__key").destroy();
            }
            catch (Exception* ex)
            {
                abort_transaction();
                throw ex;
            }
        }
        return *this;
    }
    
    Storage &Storage::check_unique(const Bson& n, const std::string& name, tokyo::DB* index)
    {
        if (Bson::k_document == n.type() &&
            nested_indexing_.end() != nested_indexing_.find(name))
        {
            Log::debug.log("checking children of [%s].") << name << Log::end;
            
            for (Linked_map<std::string, Bson*>::const_iterator iter = n.to_map().begin();
                 n.to_map().end() != iter;
                 ++iter)
            {
                char* bson = iter->second->to_binary();
                std::pair<int, int> delta(bson_to_storage_delta(iter->second));
                tokyo::DB::value_t existing = index->at(bson + delta.first,
                                                        iter->second->size() - delta.second);
                delete[] bson;
                if (existing.first)
                {
                    throw new Exception("StorageError",
                                        std::string("Unable to place record because of unique constraint [").append(name).append("]."));
                }
            }
        }
        // XXX add the array type support here.
        else
        {
            Log::debug.log("Checking value of [%s].") << name << Log::end;
            char *bson = n.to_binary();
            std::pair<int, int> delta(bson_to_storage_delta(&n));
            tokyo::DB::value_t existing = index->at(bson + delta.first,
                                                    n.size() - delta.second);
            delete[] bson;
            if (existing.first)
            {
                throw new Exception("StorageError",
                                    std::string("Unable to place record because of unique constraint [").append(name).append("]."));
            }
        }
        return *this;
    }
    
    // XXX Method is too long. Needs to be devided up somehow.
    Storage &Storage::deindex(const unsigned long long key)
    {
        if (!key)
        {
            return *this;
        }
        
        Log::debug.log("Remove [%d] from indicies.") << key << Log::end;
        Bson original;
        at(key)->first(original);
        
        // Remove from tree index entries.
        for (std::map<std::string, tokyo::Tree_db *>::const_iterator iter = fields_tree_.begin();
             fields_tree_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists() && 
                Bson::k_document == n.type() && 
                nested_indexing_.end() != nested_indexing_.find(iter->first))
            {
                for (Linked_map<std::string, Bson*>::const_iterator iter2 = n.to_map().begin();
                     n.to_map().end() != iter2;
                     ++iter2)
                {
                    char* bson = iter2->second->to_binary();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->remove_from_existing(bson + delta.first,
                                                       iter2->second->size() - delta.second,
                                                       &key,
                                                       sizeof(unsigned long long));
                    delete[] bson;
                }
            }
            // XXX add the array type support here.
            else if (n.exists())
            {
                char* bson = n.to_binary();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->remove_from_existing(bson + delta.first,
                                                   n.size() - delta.second,
                                                   &key,
                                                   sizeof(unsigned long long));
                delete[] bson;
            }
        }
        
        // remove from hash index entries.
        for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists() &&
                Bson::k_document == n.type() && 
                nested_indexing_.end() != nested_indexing_.find(iter->first))
            {
                for (Linked_map<std::string, Bson*>::const_iterator iter2 = n.to_map().begin();
                     n.to_map().end() != iter2;
                     ++iter2)
                {
                    char* bson = iter2->second->to_binary();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->remove(bson + delta.first,
                                         iter2->second->size() - delta.second);
                    delete[] bson;
                }
            }
            // XXX add the array type support here.
            else if (n.exists())
            {
                char* bson = n.to_binary();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->remove(bson + delta.first,
                                     n.size() - delta.second);
                delete[] bson;
            }
        }
        
        // remove from text searches.
        for (std::map<std::string, TextSearcher *>::const_iterator iter = fields_text_.begin();
             fields_text_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->remove(key, bson_as_string(n));
            }
        }
        
        // remove from the tag searches.
        for (std::map<std::string, TagSearcher *>::const_iterator iter = fields_tag_.begin();
             fields_tag_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->remove(key, bson_as_value_string_set(n));
            }
        }
        return *this;
    }
    
    Storage &Storage::reindex(const unsigned long long key)
    {
        if (!key)
        {
            return *this;
        }
        
        Log::debug.log("Place [%d] in indicies.") << key << Log::end;
        Bson original;
        at(key)->first(original);
        
        // Insert into tree index.
        for (std::map<std::string, tokyo::Tree_db*>::const_iterator iter = fields_tree_.begin();
             fields_tree_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists() &&
                Bson::k_document == n.type() &&
                nested_indexing_.end() != nested_indexing_.find(iter->first))
            {
                for (Linked_map<std::string, Bson*>::const_iterator iter2 = n.to_map().begin();
                     n.to_map().end() != iter2;
                     ++iter2)
                {
                    char* bson = iter2->second->to_binary();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->place_with_existing(bson + delta.first,
                                                      iter2->second->size() - delta.second,
                                                      &key,
                                                      sizeof(unsigned long long));
                    delete[] bson;
                }
            }
            // XXX add the array type support here.
            else if (n.exists())
            {
                char* bson = n.to_binary();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->place_with_existing(bson + delta.first,
                                                  n.size() - delta.second,
                                                  &key,
                                                  sizeof(unsigned long long));
                delete[] bson;
            }
        }
        
        // Insert into hash index.
        for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists() &&
                Bson::k_document == n.type() &&
                nested_indexing_.end() != nested_indexing_.find(iter->first))
            {
                for (Linked_map<std::string, Bson*>::const_iterator iter2 = n.to_map().begin();
                     n.to_map().end() != iter2;
                     ++iter2)
                {
                    char* bson = iter2->second->to_binary();
                    std::pair<int, int> delta(bson_to_storage_delta(iter2->second));
                    iter->second->place(bson + delta.first,
                                        iter2->second->size() - delta.second,
                                        &key,
                                        sizeof(unsigned long long));
                    delete[] bson;
                }
            }
            // XXX add the array type support here.
            else if (n.exists())
            {
                char* bson = n.to_binary();
                std::pair<int, int> delta(bson_to_storage_delta(&n));
                iter->second->place(bson + delta.first,
                                    n.size() - delta.second,
                                    &key,
                                    sizeof(unsigned long long));
                delete[] bson;
            }
        }
        
        // insert into text searcher
        for (std::map<std::string, TextSearcher *>::const_iterator iter = fields_text_.begin();
             fields_text_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->index(key, bson_as_string(n));
            }
        }
        
        // insert into tag searcher
        for (std::map<std::string, TagSearcher *>::const_iterator iter = fields_tag_.begin();
             fields_tag_.end() != iter;
             ++iter)
        {
            Bson n(original.nav(iter->first));
            if (n.exists())
            {
                iter->second->index(key, bson_as_value_string_set(n));
            }
        }
        return *this;
    }
    
    void Storage::begin_transaction()
    {
        db_->start_writes();
        for (std::map<std::string, tokyo::Tree_db*>::const_iterator iter = fields_tree_.begin();
             fields_tree_.end() != iter;
             ++iter)
        {
            iter->second->start_writes();
        }
        for (std::map<std::string, tokyo::Hash_db*>::const_iterator iter = fields_hash_.begin();
             fields_hash_.end() != iter;
             ++iter)
        {
            iter->second->start_writes();
        }
    }
    void Storage::commit_transaction()
    {
        for (std::map<std::string, tokyo::Hash_db*>::reverse_iterator iter = fields_hash_.rbegin();
             fields_hash_.rend() != iter;
             ++iter)
        {
            iter->second->save_writes();
        }
        for (std::map<std::string, tokyo::Tree_db *>::reverse_iterator iter = fields_tree_.rbegin();
             fields_tree_.rend() != iter;
             ++iter)
        {
            iter->second->save_writes();
        }
        db_->save_writes();
    }
    void Storage::abort_transaction()
    {
        for (std::map<std::string, tokyo::Hash_db*>::reverse_iterator iter = fields_hash_.rbegin();
             fields_hash_.rend() != iter;
             ++iter)
        {
            iter->second->abort_writes();
        }
        for (std::map<std::string, tokyo::Tree_db *>::reverse_iterator iter = fields_tree_.rbegin();
             fields_tree_.rend() != iter;
             ++iter)
        {
            iter->second->abort_writes();
        }
        db_->abort_writes();
    }
};