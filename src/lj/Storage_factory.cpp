/*!
 \file Storage_factory.cpp
 \brief LJ Storage_factory implementation.
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

#include "Storage_factory.h"

#include "lj/Storage.h"

namespace lj
{
    std::map<std::string, Storage*> Storage_factory::cache_;

    Storage* Storage_factory::produce(const std::string& name)
    {
        Storage* tmp = 0;
        Cache_map::iterator iter(cache_.find(name));
        if (cache_.end() == iter)
        {
            tmp = new Storage(name);
            cache_.insert(Cache_map::value_type(name, tmp));
        }
        else
        {
            tmp = (*iter).second;
        }
        return tmp;
    }
    
    void Storage_factory::recall(const std::string& name)
    {
        Cache_map::iterator iter(cache_.find(name));
        if (cache_.end() != iter)
        {
            delete (*iter).second;
        }
    }
    
    void Storage_factory::checkpoint_all()
    {
        for (Cache_map::iterator iter = cache_.begin();
             cache_.end() != iter;
             ++iter)
        {
            (*iter).second->checkpoint();
        }
    }


}; // namespace lj