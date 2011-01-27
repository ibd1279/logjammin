/*!
 \file lj/Storage.cpp
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
#include "lj/Engine.h"
#include "lj/engines/Tokyo_index.h"
#include "lj/engines/Tokyo_vault.h"
#include "lj/Exception.h"
#include "lj/Logger.h"

#include <cassert>
#include <cstring>
#include <sstream>
#include <sys/time.h>

// XXX This should be moved somewhere for portability.
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

// ====================================================================
// Opening a storage object.
// ====================================================================
namespace
{
    inline std::string storage_config_path(const lj::Bson* const server_config,
                                           const std::string& name)
    {
        return lj::bson_as_string(server_config->nav("server/directory")) +
               "/" + name + "/config";
    }
}; // namespace (anonymous)

namespace lj
{
    Storage::Storage(const std::string &name,
                     const lj::Bson* const server_config) :
                     vault_(NULL),
                     indices_(),
                     storage_config_(NULL),
                     server_config_(server_config)
    {
        const std::string& configfile =
                storage_config_path(server_config_, name);
        
        storage_config_ = bson_load(configfile);
        storage_config_->set_child("storage/name",
                                   lj::bson_new_string(name));

        vault_ = new lj::engines::Tokyo_vault(server_config_,
                                              storage_config_,
                                              storage_config_->path("vault"),
                                              this);

        const lj::Linked_map<std::string, lj::Bson*>& indices =
                storage_config_->nav("indices").to_map();
        std::for_each(indices.begin(), indices.end(), [&indices_, server_config_, storage_config_, this] (const std::pair<std::string, lj::Bson*>& pair)
        {
            lj::Bson* index_config = pair.second;
            const std::string& field =
                    lj::bson_as_string(index_config->nav("field"));
            lj::Index* index = new lj::engines::Tokyo_index(server_config_,
                                                            storage_config_,
                                                            index_config,
                                                            this);
            indices_.insert(std::pair<std::string, lj::Index*>(field, index));
        });
    }
    
    Storage::~Storage()
    {
        delete storage_config_;
        delete vault_;
        std::for_each(indices_.begin(), indices_.end(), [] (const std::pair<std::string, lj::Index*> pair)
        {
            delete pair.second;
        });
    }
}; // namespace lj

// ====================================================================
// indexing/deindexing a record in a storage object.
// ====================================================================
namespace
{
    std::pair<int, int> bson_to_storage_delta(const lj::Bson& item)
    {
        if (lj::bson_type_is_quotable(item.type()))
        {
            return std::pair<int, int>(4,5);
        }
        else
        {
            return std::pair<int, int>(0,0);
        }
    }

    void deindex(const std::map<std::string, lj::Index*>& indices,
                 const lj::Bson& item,
                 const lj::Uuid& uid)
    {
        std::for_each(indices.begin(), indices.end(), [&item, &uid] (const std::pair<std::string, lj::Index*>& pair)
        {
            const lj::Bson* value = item.path(pair.first);
            if (value)
            {
                lj::Index* index = pair.second;
                char* data = value->to_binary();
                auto delta = bson_to_storage_delta(*value);
                index->remove(data + delta.first,
                              value->size() - delta.second,
                              uid);
                delete[] data;
            }
        });
    }

    void reindex(const std::map<std::string, lj::Index*>& indices,
               const lj::Bson& item,
               const lj::Uuid& uid)
    {
        std::for_each(indices.begin(), indices.end(), [&item, &uid] (const std::pair<std::string, lj::Index*>& pair)
        {
            const lj::Bson* value = item.path(pair.first);
            if (value)
            {
                lj::Index* index = pair.second;
                char* data = value->to_binary();
                auto delta = bson_to_storage_delta(*value);
                index->place(data + delta.first,
                             value->size() - delta.second,
                             uid);
                delete[] data;
            }
        });
    }

    void check(const std::map<std::string, lj::Index*>& indices,
               const lj::Bson& item,
               const lj::Uuid& uid)
    {
        std::for_each(indices.begin(), indices.end(), [&item, &uid] (const std::pair<std::string, lj::Index*>& pair)
        {
            const lj::Bson* value = item.path(pair.first);
            if (value)
            {
                lj::Index* index = pair.second;
                char* data = value->to_binary();
                auto delta = bson_to_storage_delta(*value);
                index->check(data + delta.first,
                             value->size() - delta.second,
                             uid);
                delete[] data;
            }
        });
    }

}; // namespace

namespace lj
{
    const lj::Index* const Storage::index(const std::string& indx) const
    {
        auto iter = indices_.find(indx);
        if (indices_.end() == iter)
        {
            return NULL;
        }
        return (*iter).second;
    }

    const lj::Vault* const Storage::vault() const
    {
        return vault_;
    }

    Storage& Storage::place(lj::Bson& item)
    {
        Uuid uid(lj::bson_as_uuid(item["__uid"]));
        uint64_t key = lj::bson_as_int64(item["__key"]);
        const Uuid original_uid(uid);
        const uint64_t original_key = key;
        
        try
        {
            vault_->journal_begin(uid);

            if (key)
            {
                deindex(indices_, item, uid);
            }
            else
            {
                key = vault_->next_key();
                uid = Uuid(key);
                item.set_child("__uid", lj::bson_new_uuid(uid));
                item.set_child("__key", lj::bson_new_uint64(key));
            }
            
            check(indices_, item, uid);

            vault_->place(item);

            reindex(indices_, item, uid);

            vault_->journal_end(uid);
        }
        catch (lj::Exception* ex)
        {
            // XXX Rollback
            throw ex;
        }
        return *this;
    }
    
    Storage &Storage::remove(lj::Bson& item)
    {
        Uuid uid(lj::bson_as_uuid(item["__uid"]));
        uint64_t key = lj::bson_as_int64(item["__key"]);
        const Uuid original_uid(uid);
        const uint64_t original_key = key;
        
        if (key)
        {
            try
            {
                vault_->journal_begin(uid);

                deindex(indices_, item, uid);

                vault_->remove(item);

                vault_->journal_end(uid);

                item["__key"].destroy();
                item["__uid"].destroy();
            }
            catch (Exception* ex)
            {
                // XXX Rollback
                throw ex;
            }
        }
        return *this;
    }
    
    
    const lj::Bson* const Storage::storage_config()
    {
        return storage_config_;
    }
    
    //======================================================================
    // Storage configuration methods.
    
    void storage_config_init(lj::Bson& cfg,
                             const std::string& name)
    {
        cfg.set_child("main/name", lj::bson_new_string(name));
        cfg.set_child("main/compare", lj::bson_new_string("int64"));
        cfg.set_child("main/file", lj::bson_new_string(std::string("db.") + name + ".tcb"));
        cfg.set_child("main/backup_file", lj::bson_new_string(std::string("db.") + name + ".tcb.backup"));
        cfg.set_child("journal/file", lj::bson_new_string(std::string("journal.") + name + ".tcf"));
        cfg.set_child("journal/type", lj::bson_new_string("fixed"));
    }
    
    void storage_config_add_index(lj::Bson& cfg,
                                  const std::string& type,
                                  const std::string& field,
                                  const std::string& comp)
    {
        std::string extension;
        if (type.compare("tree") == 0)
        {
            extension = "tcb";
        }
        else if (type.compare("hash") == 0)
        {
            extension = "tch";
        }
        else if (type.compare("text") == 0)
        {
            extension = "tcq";
        }
        else if (type.compare("tag") == 0)
        {
            extension = "tcw";
        }
        else {
            extension = type + ".tc";
        }
        
        std::string name;
        for (std::string::const_iterator iter = field.begin();
             field.end() != iter;
             ++iter)
        {
            if ('/' == *iter)
            {
                // We push an escae before the slash to avoid confusion in set_child.
                name.push_back('~');
            }
            else
            {
                name.push_back(*iter);
            }
        }
        
        lj::Bson* index_cfg = new lj::Bson();
        cfg.set_child(std::string("index/") + type + "/" + name,
                      index_cfg);
        
        index_cfg->set_child("compare", lj::bson_new_string(comp));
        index_cfg->set_child("file", lj::bson_new_string(std::string("index.") + name + "." + extension));
        index_cfg->set_child("type", lj::bson_new_string(type));
        index_cfg->set_child("field", lj::bson_new_string(field));
    }
    
    void storage_config_remove_index(lj::Bson& cfg,
                                     const std::string& type,
                                     const std::string& field)
    {
        std::string name;
        for (std::string::const_iterator iter = field.begin();
             field.end() != iter;
             ++iter)
        {
            if ('/' == *iter)
            {
                // We push an escae before the slash to avoid confusion in set_child.
                name.push_back('~');
            }
            else
            {
                name.push_back(*iter);
            }
        }
        
        lj::Bson* index_cfg = cfg.path(std::string("index/") + type + "/" + name);
        index_cfg->destroy();
    }
    
    void storage_config_add_subfield(lj::Bson& cfg,
                                     const std::string& field)
    {
        lj::Bson* ptr = cfg.path("main/nested");
        std::set<std::string> allowed(lj::bson_as_value_string_set(*ptr));
        allowed.insert(field);
        
        ptr->destroy();
        for (std::set<std::string>::const_iterator iter = allowed.begin();
             allowed.end() != iter;
             ++iter)
        {
            ptr->push_child("", lj::bson_new_string(*iter));
        }
    }
    
    void storage_config_save(const lj::Bson& cfg, const lj::Bson& server_config)
    {
        std::string dbfile(root_data_directory(server_config));
        
        const std::string& dbname = lj::bson_as_string(cfg.nav("main/name"));
        dbfile.append("/").append(dbname);
        
        // XXX This should be moved somewhere for portability.
        int err = mkdir(dbfile.c_str(), S_IRWXU | S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP);
        if (0 != err && EEXIST != errno)
        {
            throw new lj::Exception("StorageConfigSave", strerror(errno));
        }
        
        dbfile.append("/config");
        lj::bson_save(cfg, dbfile);
    }
    
    lj::Bson* storage_config_load(const std::string& dbname, const lj::Bson& server_config)
    {
        lj::Bson* ptr = lj::bson_load(storage_config_path(server_config, dbname));
        return ptr;
    }
};
