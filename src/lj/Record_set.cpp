/*!
 \file lj/Record_set.cpp
 \brief LJ Record_set implementation.
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

#include "lj/Record_set.h"

#include "lj/Storage.h"

namespace lj {
    tokyo::Tree_db* Record_set::storage_db(const Storage* s)
    {
        return s->db_;
    }
    
    tokyo::Tree_db* Record_set::storage_tree(const Storage* s,
                                             const std::string& indx)
    {
        std::map<std::string, tokyo::Tree_db*>::const_iterator i = s->fields_tree_.find(indx);
        if (s->fields_tree_.end() == i)
        {
            return 0;
        }
        return (*i).second;
    }
    
    tokyo::Hash_db* Record_set::storage_hash(const Storage* s,
                                             const std::string& indx)
    {
        std::map<std::string, tokyo::Hash_db*>::const_iterator i = s->fields_hash_.find(indx);
        if (s->fields_hash_.end() == i)
        {
            return 0;
        }
        return (*i).second;
    }
    
    tokyo::TextSearcher* Record_set::storage_text(const Storage* s,
                                                  const std::string& indx)
    {
        std::map<std::string, tokyo::TextSearcher*>::const_iterator i = s->fields_text_.find(indx);
        if (s->fields_text_.end() == i)
        {
            return 0;
        }
        return (*i).second;
    }
    
    tokyo::TagSearcher* Record_set::storage_tag(const Storage* s,
                                                const std::string& indx)
    {
        std::map<std::string, tokyo::TagSearcher*>::const_iterator i = s->fields_tag_.find(indx);
        if (s->fields_tag_.end() == i)
        {
            return 0;
        }
        return (*i).second;
    }
    
    void Record_set::list_to_set(const tokyo::DB::list_value_t& a,
                                 std::set<unsigned long long>& b)
    {
        for (tokyo::DB::list_value_t::const_iterator iter = a.begin();
             a.end() != iter;
             ++iter)
        {
            unsigned long long* x = static_cast<unsigned long long*>(iter->first);
            if (x)
            {
                b.insert(*x);
                free(iter->first);
            }
        }
    }
}; // namespace lj
