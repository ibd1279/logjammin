/*
 \file Release.cpp
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

#include "Release.h"

namespace logjammin {
    namespace {
        /**************************************************************************
         * RELEASE Database
         *************************************************************************/
        
        const char RELEASE_DB[] = "/var/db/logjammin/release.tcb";
        const char RELEASE_INDX_NATURAL[] = "/var/db/logjammin/release_natural.tcb";
        const char RELEASE_SRCH_NAME[] = "/var/db/logjammin/backlog_name";
        
        //! Release Database Class.
        /*!
         Stores the release records.
         \author Jason Watson
         \version 1.0
         \date February 1, 2010
         */
        class ReleaseDB : public ModelDB<Release> {
            static void open_db_file(TCBDB *db, int mode) {
                tcbdbsetcmpfunc(db, tccmpint64, NULL);
                tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
                tcbdbopen(db, RELEASE_DB, mode);
            }
            static void open_index_file_natural(TCBDB *db, int mode) {
                tcbdbsetcmpfunc(db, tccmplexical, NULL);
                tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
                tcbdbopen(db, RELEASE_INDX_NATURAL, mode);
            }
            static void open_search_file_name(TCIDB *db, int mode) {
                tcidbtune(db, -1, -1, -1, IDBTLARGE | IDBTBZIP);
                tcidbopen(db, RELEASE_SRCH_NAME, mode);
            }            
        public:
            static ReleaseDB *instance() {
                static ReleaseDB dao;
                return &dao;
            }
            
            tokyo::Index<unsigned long long, std::string> index_natural;
            tokyo::Search<unsigned long long> search_name;
            
            ReleaseDB() :
            ModelDB<Release>(&open_db_file, BDBOREADER | BDBOWRITER | BDBOCREAT),
            index_natural(&open_index_file_natural, BDBOREADER | BDBOWRITER | BDBOCREAT),
            search_name(&open_search_file_name, IDBOREADER | IDBOWRITER | IDBOCREAT)
            {
            }
            
            virtual void put(Release *model) {
                try {
                    begin_transaction();
                    index_natural.begin_transaction();
                    
                    if (model->pkey() != 0) {
                        const Release r(model->pkey());
                        
                        //clean indicies
                        index_natural.remove(r.natural_key(), model->pkey());
                    }
                    
                    // Verify that the unique constraints.
                    std::set<unsigned long long> tmp = index_natural.is(model->natural_key());
                    throw tokyo::Exception("Constraint error",
                                           "Natural key already exists in release database.");
                    
                    
                    // Get the primary key for new objects.
                    unsigned long long key = model->pkey();
                    if (key == 0) {
                        try {
                            key = max() + 1;
                        } catch(tokyo::Exception &ex) {
                            key = 1;
                        }
                    }
                    
                    // Store the records in the databases and indicies.
                    this->_put(key, model->serialize());
                    index_natural.put(model->natural_key(), key);
                    search_name.index(model->name(), key);
                    
                    index_natural.commit_transaction();
                    commit_transaction();
                    
                    set_pkey(model, key);
                } catch (tokyo::Exception &ex) {
                    index_natural.abort_transaction();
                    abort_transaction();
                    throw ex;
                } catch (std::string &ex) {
                    index_natural.abort_transaction();
                    abort_transaction();
                    throw ex;
                }
            }
            
            virtual void remove(Release *model) {
                if (model->pkey() != 0) {
                    try {
                        begin_transaction();
                        index_natural.begin_transaction();
                        
                        // Get the original records.
                        const Release r(model->pkey());
                        this->_remove(model->pkey());
                        index_natural.remove(r.natural_key(), model->pkey());
                        search_name.remove(model->pkey());
                        
                        index_natural.commit_transaction();
                        commit_transaction();
                        
                        set_pkey(model, 0);
                    } catch (tokyo::Exception &ex) {
                        index_natural.abort_transaction();
                        abort_transaction();
                        throw ex;
                    } catch (std::string &ex) {
                        index_natural.abort_transaction();
                        abort_transaction();
                        throw ex;
                    }
                }
            }
        };
        
        int Release_project(Release *obj, lua_State *L) {
            Lunar<Project>::push(L, new Project(obj->project().pkey()), true);
            return 1;
        }
        
        int Release_tasks(Release *obj, lua_State *L) {
            lua_newtable(L);
            int i = 0;
            for (std::list<Backlog>::const_iterator iter = obj->tasks().begin();
                 iter != obj->tasks().end();
                 ++iter) {
                Lunar<Backlog>::push(L, new Backlog(iter->pkey()), true);
                lua_rawseti(L, -2, ++i);
            }
            return 1;
        }
    };
    
    const char Release::LUNAR_CLASS_NAME[] = "Release";
    Lunar<Release>::RegType Release::LUNAR_METHODS[] = {
    LUNAR_STRING_GETTER(Release, name),
    LUNAR_STATIC_METHOD(Release, project),
    LUNAR_STATIC_METHOD(Release, tasks),
    {0,0,0,0}
    };
    

    /******************************************************************************
     * Release Methods
     *****************************************************************************/
    
    std::list<Release *> Release::all(const Project &project,
                                      const std::string &version) {
        std::ostringstream nkey;
        nkey << project.pkey();
        if(version.size() > 0) {
            nkey << "::" << version;
        }
        
        std::set<unsigned long long> keys(ReleaseDB::instance()->index_natural.starts(nkey.str()));
        
        std::list<Release *> results;
        for(std::set<unsigned long long>::const_iterator iter = keys.begin();
            iter != keys.end();
            ++iter) {
            results.push_back(new Release(*iter));
        }
        return results;
    }
    
    std::list<Release *> Release::like(const std::string &term,
                                       const Project &project,
                                       const std::string &version) {
        std::ostringstream nkey;
        nkey << project.pkey();
        if(version.size() > 0) {
            nkey << "::" << version;
        }
        
        std::set<unsigned long long> scope_set(ReleaseDB::instance()->index_natural.starts(nkey.str()));
        
        std::set<unsigned long long> search_set;
        ReleaseDB::instance()->search_name.like(term, search_set);
        
        std::list<Release *> results;
        for(std::set<unsigned long long>::const_iterator iter = search_set.begin();
            iter != search_set.end();
            ++iter) {
            if(scope_set.find(*iter) != scope_set.end()) {
                results.push_back(new Release(*iter));
            }
        }
        return results;
    }
    
    void Release::at(const unsigned long long key, Release *model) {
        ReleaseDB::instance()->at(key, model);
    }
    
    Release::Release() : _name(), _project(), _tasks() {
    }
    
    Release::Release(const unsigned long long key) {
        Release::at(key, this);
    }
    
    Release::Release(lua_State *L) : _name(), _project(), _tasks() {
    }
    
    Release::Release(const Release &orig) : Model<Release>(orig),
    _name(orig._name), _project(orig._project), _tasks(orig._tasks) {
    }
    
    Release::~Release() {
    }
    
    std::string Release::natural_key() const {
        std::ostringstream data;
        data << project().pkey() << "::" << version();
        data << "::" << name();
        return data.str();
    }
    
    void Release::populate(OpenProp::File *props) {
        if (props->getValue("name").exists())
            name(std::string(props->getValue("name")));
        
        if (props->getValue("project").exists())
            project(Project((long)props->getValue("project")));
        
        OpenProp::ElementIterator *iter = props->getElement("tasks")->getElements();
        _tasks.clear();
        while(iter->more())
            _tasks.push_back(Backlog((long)iter->next()->getValue()));
    }
    
    const std::string Release::serialize() const {
        std::ostringstream data;
        int i = 0;
        
        data << "name=\"" << escape(_name) << "\";\n";
        data << "version=\"" << escape(_version) << "\";\n";
        data << "project=\"" << _project.pkey() << "\";\n";
        data << "tasks{\n";
        for(std::list<Backlog>::const_iterator iter = _tasks.begin();
            iter != _tasks.end();
            ++iter, ++i) {
            data << "    t" << i << "=\"" << iter->pkey() << "\";\n";
        }
        data << "};\n";
        
        return data.str();
    }
    
    ModelDB<Release> *Release::dao() const {
        return ReleaseDB::instance();
    }
};