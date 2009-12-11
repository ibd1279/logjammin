/*
 \file Project.cpp
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

#include "Project.h"

namespace logjammin {
    namespace {
        /**************************************************************************
         * Project Database
         *************************************************************************/
        
        const char PROJECT_DB[] = "/var/db/logjammin/project.tcb";
        const char PROJECT_INDX_NAME[] = "/var/db/logjammin/project_name.tcb";
        const char PROJECT_SRCH_CATEGORY[] = "/var/db/logjammin/project_categories";
        const char PROJECT_SRCH_VERSION[] = "/var/db/logjammin/project_versions";
        const char PROJECT_SRCH_NAME[] = "/var/db/logjammin/project_name";
        
        //! Project Database Class.
        /*!
         Stores the project records and creates an exact index on the name.
         \author Jason Watson
         \version 1.0
         \date July 3, 2009
         */
        class ProjectDB : public ModelDB<Project> {
            static void open_db_file(TCBDB *db, int mode){
                tcbdbsetcmpfunc(db, tccmpint64, NULL);
                tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
                tcbdbopen(db, PROJECT_DB, mode);
            }
            static void open_indx_file_name(TCBDB *db, int mode) {
                tcbdbsetcmpfunc(db, tccmplexical, NULL);
                tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
                tcbdbopen(db, PROJECT_INDX_NAME, mode);
            }
            static void open_search_file_category(TCJDB *db, int mode) {
                tcjdbtune(db, -1, -1, -1, JDBTLARGE | JDBTBZIP);
                tcjdbopen(db, PROJECT_SRCH_CATEGORY, mode);
            }
            static void open_search_file_version(TCJDB *db, int mode) {
                tcjdbtune(db, -1, -1, -1, JDBTLARGE | JDBTBZIP);
                tcjdbopen(db, PROJECT_SRCH_VERSION, mode);
            }
            static void open_search_file_name(TCIDB *db, int mode) {
                tcidbtune(db, -1, -1, -1, IDBTLARGE | IDBTBZIP);
                tcidbopen(db, PROJECT_SRCH_NAME, mode);
            }
        public:
            static ProjectDB* instance() {
                static ProjectDB dbo;
                return &dbo;
            }
            
            tokyo::Index<unsigned long long, std::string> index_name;
            tokyo::Search<unsigned long long> search_name;
            tokyo::Tags<unsigned long long> search_category, search_version;
            
            //! Create a new Project DB Object.
            ProjectDB() :
            ModelDB<Project>(&open_db_file, BDBOREADER | BDBOWRITER | BDBOCREAT),
            index_name(&open_indx_file_name, BDBOREADER | BDBOWRITER | BDBOCREAT),
            search_category(&open_search_file_category, JDBOREADER | JDBOWRITER | JDBOCREAT),
            search_version(&open_search_file_version, JDBOREADER | JDBOWRITER | JDBOCREAT),
            search_name(&open_search_file_name, IDBOREADER | IDBOWRITER | IDBOCREAT)
            {
            }
            
            //! Put a new record into the database.
            virtual void put(Project *model) {
                try {
                    begin_transaction();
                    index_name.begin_transaction();
                    
                    // Clean up the index.
                    if(model->pkey() != 0) {
                        Project p(model->pkey());
                        index_name.remove(p.name(), model->pkey());
                    }
                    
                    // Make sure the name isn't used elsewhere.
                    std::set<unsigned long long> tmp = index_name.is(model->name());
                    if(tmp.size() != 0)
                        throw tokyo::Exception("Constraint error", "Name already exists in project database.");
                    
                    // Get the primary key for new objects.
                    unsigned long long key = model->pkey();
                    if(key == 0) {
                        try {
                            key = max() + 1;
                        } catch(tokyo::Exception &ex) {
                            key = 1;
                        }
                    }
                    
                    // Store the records.
                    this->_put(key, model->serialize());
                    index_name.put(model->name(), key);
                    search_category.index(std::set<std::string>(model->categories().begin(), model->categories().end()), key);
                    search_version.index(std::set<std::string>(model->versions().begin(), model->versions().end()), key);
                    search_name.index(model->name(), key);
                    
                    index_name.commit_transaction();
                    commit_transaction();
                    
                    set_pkey(model, key);
                } catch (tokyo::Exception &ex) {
                    index_name.abort_transaction();
                    abort_transaction();
                    throw ex;
                } catch (std::string &ex) {
                    index_name.abort_transaction();
                    abort_transaction();
                    throw ex;
                }
            }
            
            //! Remove a record from the Project DB.
            virtual void remove(Project *model) {
                if(model->pkey() != 0) {
                    try {
                        begin_transaction();
                        index_name.begin_transaction();
                        
                        Project p(model->pkey());
                        this->_remove(model->pkey());
                        index_name.remove(p.name(), model->pkey());
                        search_category.remove(model->pkey());
                        search_version.remove(model->pkey());
                        search_name.remove(model->pkey());
                        
                        search_name.optimize();
                        index_name.commit_transaction();
                        commit_transaction();
                        
                        set_pkey(model, 0);
                    } catch (tokyo::Exception &ex) {
                        index_name.abort_transaction();
                        abort_transaction();
                        throw ex;
                    } catch (std::string &ex) {
                        index_name.abort_transaction();
                        abort_transaction();
                        throw ex;
                    }
                }
            }
        };
        
        
        /**************************************************************************
         * Project Lua Integration.
         *************************************************************************/
        
        int Project_categories(Project *obj, lua_State *L) {
            lua_newtable(L);
            int i = 0;
            std::list<std::string> cats = obj->categories();
            for(std::list<std::string>::const_iterator iter = cats.begin(); iter != cats.end(); ++iter) {
                lua_pushstring(L, iter->c_str());
                lua_rawseti(L, -2, ++i);
            }
            return 1;
        }
        
        int Project_versions(Project *obj, lua_State *L) {
            lua_newtable(L);
            int i = 0;
            std::list<std::string> vers = obj->versions();
            for(std::list<std::string>::const_iterator iter = vers.begin(); iter != vers.end(); ++iter) {
                lua_pushstring(L, iter->c_str());
                lua_rawseti(L, -2, ++i);
            }
            return 1;
        }
    }; // namespace
    
    const char Project::LUNAR_CLASS_NAME[] = "Project";
    Lunar<Project>::RegType Project::LUNAR_METHODS[] = {
    LUNAR_STRING_GETTER(Project, name),
    LUNAR_STRING_GETTER(Project, commit_feed),
    LUNAR_STATIC_METHOD(Project, categories),
    LUNAR_STATIC_METHOD(Project, versions),
    LUNAR_INTEGER_GETTER(Project, pkey, unsigned long long),
    {0,0,0,0}
    };
    
    
    /******************************************************************************
     * Project methods
     *****************************************************************************/
    
    std::list<Project *> Project::all() {
        std::list<Project *> results;
        ProjectDB::instance()->all(results);
        return results;
    }
    
    std::list<Project *> Project::like(const std::string &term) {
        std::set<unsigned long long> keys;
        ProjectDB::instance()->search_name.like(term, keys);
        ProjectDB::instance()->search_version.tagged(term, keys);
        ProjectDB::instance()->search_category.tagged(term, keys);
        
        std::list<Project *> results;
        for(std::set<unsigned long long>::const_iterator iter = keys.begin();
            iter != keys.end();
            ++iter) {
            results.push_back(new Project(*iter));
        }
        
        return results;
    }
    
    void Project::at_name(const std::string &name, Project *model) {
        std::set<unsigned long long> pkeys(ProjectDB::instance()->index_name.is(name));
        if(pkeys.size() == 0)
            throw std::string("Unknown Project Name ").append(name).append(".");
        else if(pkeys.size() > 1)
            throw std::string("Ambiguous Project Name ").append(name).append(".");
        
        ProjectDB::instance()->at(*(pkeys.begin()), model);
    }
    
    void Project::at(const unsigned long long key, Project *model) {
        ProjectDB::instance()->at(key, model);
    }
    
    Project::Project() {
    }
    
    Project::Project(unsigned long long key) {
        Project::at(key, this);
    }
    
    Project::Project(const std::string &name) {
        Project::at_name(name, this);
    }
    
    Project::Project(lua_State *L) {
    }
    
    Project::~Project() {
    }
    
    void Project::populate(OpenProp::File *props) {
        if(props->getValue("name").exists())
            name(std::string(props->getValue("name")));
        
        if(props->getValue("feed").exists())
            commit_feed(std::string(props->getValue("feed")));
        
        OpenProp::ElementIterator *iter = props->getElement("versions")->getElements();
        _versions.clear();
        while(iter->more())
            _versions.push_back(std::string(iter->next()->getValue()));
        
        iter = props->getElement("categories")->getElements();
        _categories.clear();
        while(iter->more())
            _categories.push_back(std::string(iter->next()->getValue()));
    }
    
    const std::string Project::serialize() const {
        std::ostringstream data;
        int i = 0, j = 0;
        
        data << "name=\"" << escape(_name) << "\";\n";
        data << "feed=\"" << escape(_commit_feed) << "\";\n";
        data << "versions{\n";
        for(std::list<std::string>::const_iterator iter = _versions.begin(); iter != _versions.end(); ++iter, ++i) {
            data << "    v" << i << "=\"" << escape(*iter) << "\";\n";
        }
        data << "};\n";
        data << "categories{\n";
        for(std::list<std::string>::const_iterator iter = _categories.begin(); iter != _categories.end(); ++iter, ++j) {
            data << "    c" << j << "=\"" << escape(*iter) << "\";\n";
        }
        data << "};\n";
        
        return data.str();
    }
    
    ModelDB<Project> *Project::dao() const {
        return ProjectDB::instance();
    }
}; // namespace logjammin
