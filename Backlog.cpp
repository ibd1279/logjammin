/*
 \file Backlog.cpp
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

#include "Backlog.h"
#include "Project.h"

namespace {
    /**************************************************************************
     * Backlog Database
     *************************************************************************/
    
    const char BACKLOG_DB[] = "/var/db/logjammin/backlog.tcb";
    const char BACKLOG_INDX_NATURAL[] = "/var/db/logjammin/backlog_natural.tcb";
    const char BACKLOG_SRCH_NAME[] = "/var/db/logjammin/backlog_name";
    const char BACKLOG_SRCH_STORY[] = "/var/db/logjammin/backlog_story";
    const char BACKLOG_SRCH_COMMENTS[] = "/var/db/logjammin/backlog_comments";
    const char BACKLOG_SRCH_TAGS[] = "/var/db/logjammin/backlog_tags";

    //! Backlog Database Class.
    /*!
     Stores the backlog records.
     \author Jason Watson
     \version 1.0
     \date July 3, 2009
     */
    class BacklogDB : public ModelDB<Backlog> {
        static void open_db_file(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmpint64, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, BACKLOG_DB, mode);
        }
        static void open_index_file_natural(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmplexical, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, BACKLOG_INDX_NATURAL, mode);
        }
        static void open_search_file_name(TCIDB *db, int mode) {
            tcidbtune(db, -1, -1, -1, IDBTLARGE | IDBTBZIP);
            tcidbopen(db, BACKLOG_SRCH_NAME, mode);
        }
        static void open_search_file_story(TCIDB *db, int mode) {
            tcidbtune(db, -1, -1, -1, IDBTLARGE | IDBTBZIP);
            tcidbopen(db, BACKLOG_SRCH_STORY, mode);
        }
        static void open_search_file_comments(TCIDB *db, int mode) {
            tcidbtune(db, -1, -1, -1, IDBTLARGE | IDBTBZIP);
            tcidbopen(db, BACKLOG_SRCH_COMMENTS, mode);
        }
        static void open_tags_file_tags(TCJDB *db, int mode) {
            tcjdbtune(db, -1, -1, -1, JDBTLARGE | JDBTBZIP);
            tcjdbopen(db, BACKLOG_SRCH_TAGS, mode);
        }
    public:
        tokyo::Index<unsigned long long, std::string> index_natural;
        tokyo::Search<unsigned long long> search_name, search_story, search_comments;
        tokyo::Tags<unsigned long long> search_tags;
        
        //! Create a new Backlog DB Object.
        BacklogDB() : 
        ModelDB<Backlog>(open_db_file, BDBOREADER | BDBOWRITER | BDBOCREAT),
        index_natural(open_index_file_natural, BDBOREADER | BDBOWRITER | BDBOCREAT),
        search_name(open_search_file_name, IDBOREADER | IDBOWRITER | IDBOCREAT),
        search_story(open_search_file_story, IDBOREADER | IDBOWRITER | IDBOCREAT),
        search_comments(open_search_file_comments, IDBOREADER | IDBOWRITER | IDBOCREAT),
        search_tags(open_tags_file_tags, JDBOREADER | JDBOWRITER | JDBOCREAT)
        {
        }
        
        virtual void put(Backlog *model) {
            try {
                begin_transaction();
                index_natural.begin_transaction();
                
                // Clean up the index.
                if(model->pkey() != 0) {
                    Backlog b(model->pkey());
                    index_natural.remove(b.natural_key(), model->pkey());
                }
                
                // Make sure the name isn't used elsewhere.
                std::set<unsigned long long> tmp = index_natural.is(model->natural_key());
                if(tmp.size() != 0)
                    throw tokyo::Exception("Constraint error",
                                           "Natural key already exists in backlog database.");
                
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
                index_natural.put(model->natural_key(), key);
                search_name.index(model->brief(), key);
                search_story.index(model->story(), key);
                
                // Put together the search terms for the comments.
                std::string comments;
                for(std::list<std::string>::const_iterator iter = model->comments().begin();
                    iter != model->comments().end();
                    ++iter) {
                    comments.append(*iter);
                }
                search_comments.index(comments, key);
                
                // Tags.
                std::set<std::string> full_tags(model->tags());
                full_tags.insert(model->category());
                full_tags.insert(model->version());
                search_tags.index(full_tags, key);
                
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
        
        virtual void remove(Backlog *model) {
            if(model->pkey() != 0) {
                try {
                    begin_transaction();
                    index_natural.begin_transaction();
                    
                    Backlog b(model->pkey());
                    this->_remove(model->pkey());
                    index_natural.remove(b.natural_key(), model->pkey());
                    search_name.remove(model->pkey());
                    search_story.remove(model->pkey());
                    search_comments.remove(model->pkey());
                    search_tags.remove(model->pkey());
                    
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
    
    
    /**************************************************************************
     * Backlog Lua integration
     *************************************************************************/
    
    int Backlog_project(Backlog *obj, lua_State *L) {
        Lunar<Project>::push(L, &(obj->project()), false);
        return 1;
    }
    
    int Backlog_user(Backlog *obj, lua_State *L) {
        Lunar<User>::push(L, &(obj->user()), false);
        return 1;
    }
    
    int Backlog_tags(Backlog *obj, lua_State *L) {
        lua_newtable(L);
        int i = 0;
        std::set<std::string> tags = obj->tags();
        for(std::set<std::string>::const_iterator iter = tags.begin();
            iter != tags.end();
            ++iter) {
            lua_pushstring(L, iter->c_str());
            lua_rawseti(L, -2, ++i);
        }
        return 1;
    }
    
    int Backlog_comments(Backlog *obj, lua_State *L) {
        lua_newtable(L);
        int i = 0;
        std::list<std::string> comments = obj->comments();
        for(std::list<std::string>::reverse_iterator iter = comments.rbegin();
            iter != comments.rend();
            ++iter) {
            lua_pushstring(L, iter->c_str());
            lua_rawseti(L, -2, ++i);
        }
        return 1;
    }
};

const char Backlog::LUNAR_CLASS_NAME[] = "Backlog";
Lunar<Backlog>::RegType Backlog::LUNAR_METHODS[] = {
LUNAR_STRING_GETTER(Backlog, brief),
LUNAR_STATIC_METHOD(Backlog, project),
LUNAR_STRING_GETTER(Backlog, category),
LUNAR_STRING_GETTER(Backlog, version),
LUNAR_STRING_GETTER(Backlog, story),
LUNAR_STATIC_METHOD(Backlog, user),
LUNAR_STRING_GETTER(Backlog, disposition),
LUNAR_NUMBER_GETTER(Backlog, estimate, double),
LUNAR_STRING_GETTER(Backlog, natural_key),
LUNAR_INTEGER_GETTER(Backlog, pkey, unsigned long long),
LUNAR_STATIC_METHOD(Backlog, comments),
LUNAR_STATIC_METHOD(Backlog, tags),
{0,0,0,0}
};


/******************************************************************************
 * Backlog Database
 *****************************************************************************/    

std::list<Backlog *> Backlog::all(const Project &project,
                                  const std::string &version,
                                  const std::string &category) {
    std::ostringstream nkey;
    nkey << project.pkey();
    if(version.size() > 0) {
        nkey << "::" << version;
        if(category.size() > 0) {
            nkey << "::" << category;
        }
    }
    
    BacklogDB dao;
    std::set<unsigned long long> keys(dao.index_natural.starts(nkey.str()));
    std::list<Backlog *> results;
    for(std::set<unsigned long long>::const_iterator iter = keys.begin();
        iter != keys.end();
        ++iter) {
        results.push_back(new Backlog(*iter));
    }
    return results;
}

std::list<Backlog *> Backlog::like(const std::string &term,
                                   const Project &project,
                                   const std::string &version,
                                   const std::string &category) {
    std::ostringstream nkey;
    nkey << project.pkey();
    if(version.size() > 0) {
        nkey << "::" << version;
        if(category.size() > 0) {
            nkey << "::" << category;
        }
    }
    
    BacklogDB dao;
    // Figure out what exists in the current scope.
    std::set<unsigned long long> scope_set(dao.index_natural.starts(nkey.str()));
    
    // Search for all matches.
    std::set<unsigned long long> search_set;
    dao.search_name.like(term, search_set);
    dao.search_story.like(term, search_set);
    dao.search_comments.like(term, search_set);
    dao.search_tags.tagged(term, search_set);
    
    std::list<Backlog *> results;
    for(std::set<unsigned long long>::const_iterator iter = search_set.begin();
        iter != search_set.end();
        ++iter) {
        if(scope_set.find(*iter) != scope_set.end()) {
            // this key exists in both sets, add it to the list.
            results.push_back(new Backlog(*iter));
        }
    }
    return results;
}

void Backlog::at(const unsigned long long key, Backlog *model) {
    BacklogDB dao;
    dao.at(key, model);
}

Backlog::Backlog() {
}

Backlog::Backlog(const unsigned long long key) {
    Backlog::at(key, this);
}

Backlog::Backlog(lua_State *L) {
}

Backlog::~Backlog() { 
}

void Backlog::story(const std::string &s) {
    _story = s;
    _brief.clear();
    for(std::string::const_iterator iter = _story.begin();
        iter != _story.end();
        ++iter) {
        _brief.push_back(*iter);
        if(*iter == '.') break;
    }
}

std::string Backlog::natural_key() const {
    std::ostringstream data;
    data << project().pkey() << "::" << version() << "::";
    data << category() << "::" << brief();
    return data.str();
}

const std::string Backlog::serialize() const {
    std::ostringstream data;
    int i = 0, j = 0;
    
    data << "category=\"" << escape(_category) << "\";\n";
    data << "version=\"" << escape(_version) << "\";\n";
    data << "project=\"" << _project.pkey() << "\";\n";
    data << "story=\"" << escape(_story) << "\";\n";
    data << "user=\"" << _user.pkey() << "\";\n";
    data << "disposition=\"" << escape(_disposition) << "\";\n";
    data << "estimate=\"" << _estimate << "\";\n";
    data << "comments{\n";
    for(std::list<std::string>::const_iterator iter = _comments.begin();
        iter != _comments.end();
        ++iter, ++i) {
        data << "    c" << i << "=\"" << escape(*iter) << "\";\n";
    }
    data << "};\n";
    data << "tags{\n";
    for(std::set<std::string>::const_iterator iter = _tags.begin();
        iter != _tags.end();
        ++iter, ++j) {
        data << "    t" << j << "=\"" << escape(*iter) << "\";\n";
    }
    data << "};\n";
    
    return data.str();
}

void Backlog::populate(OpenProp::File *props) {
    category(std::string(props->getValue("category")));
    version(std::string(props->getValue("version")));
    project(Project((long)props->getValue("project")));
    story(std::string(props->getValue("story")));
    user(User((long)props->getValue("user")));
    disposition(std::string(props->getValue("disposition")));
    estimate(props->getValue("estimate"));
    
    OpenProp::ElementIterator *iter = props->getElement("comments")->getElements();
    _comments.clear();
    while(iter->more())
        _comments.push_back(std::string(iter->next()->getValue()));
    
    iter = props->getElement("tags")->getElements();
    _tags.clear();
    while(iter->more())
        _tags.insert(std::string(iter->next()->getValue()));
}

ModelDB<Backlog> *Backlog::dao() const {
    return new BacklogDB();
}
