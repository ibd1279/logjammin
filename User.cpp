#include "User.h"
#include "sha1.h"
#include <iostream>

namespace {
    /**************************************************************************
     * User Database
     *************************************************************************/
    
    const char USER_DB[] = "/var/db/logjammin/user.tcb";
    const char USER_INDX_LOGIN[] = "/var/db/logjammin/user_login.tcb";
    const char USER_SRCH_NAME[] = "/var/db/logjammin/user_name";
    const char USER_SRCH_EMAIL[] = "/var/db/logjammin/user_email";

    class UserDB : public ModelDB<User> {
        static void open_db_file(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmpint64, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, USER_DB, mode);
        }
        static void open_indx_file_login(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmplexical, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, USER_INDX_LOGIN, mode);
        }
        static void open_search_file_name(TCIDB *db, int mode) {
            tcidbtune(db, -1, -1, -1, IDBTLARGE | IDBTBZIP);
            tcidbopen(db, USER_SRCH_NAME, mode);
        }
        static void open_search_file_email(TCIDB *db, int mode) {
            tcidbtune(db, -1, -1, -1, IDBTLARGE | IDBTBZIP);
            tcidbopen(db, USER_SRCH_EMAIL, mode);
        }
    public:
        tokyo::Index<unsigned long long, std::string> index_login;
        tokyo::Search<unsigned long long> search_name, search_email;
        
        UserDB() :
        ModelDB<User>(&open_db_file, BDBOREADER | BDBOWRITER | BDBOCREAT),
        index_login(&open_indx_file_login, BDBOREADER | BDBOWRITER | BDBOCREAT),
        search_name(&open_search_file_name, IDBOREADER | IDBOWRITER | IDBOCREAT),
        search_email(&open_search_file_email, IDBOREADER | IDBOWRITER | IDBOCREAT)
        {
        }
        
        virtual void put(User *model) {
            try {
                begin_transaction();
                index_login.begin_transaction();
                
                // Clean up the index.
                if(model->pkey() != 0) {
                    User u(model->pkey());
                    for(std::list<std::string>::const_iterator iter = u.logins().begin();
                        iter != u.logins().end();
                        ++iter) {
                        index_login.remove(*iter, model->pkey());
                    }
                }
                
                // Make sure the logins we want to use aren't used.
                for(std::list<std::string>::const_iterator iter = model->logins().begin();
                    iter != model->logins().end();
                    ++iter) {
                    std::set<unsigned long long> tmp = index_login.is(*iter);
                    if(tmp.size() != 0)
                        throw tokyo::Exception("Constraint error",
                                               std::string("Login ")
                                               .append(*iter)
                                               .append(" already exists in user database.")
                                               .c_str());
                }
                
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
                for(std::list<std::string>::const_iterator iter = model->logins().begin();
                    iter != model->logins().end();
                    ++iter) {
                    index_login.put(*iter, key);
                }
                search_name.index(model->name(), key);
                search_email.index(model->email(), key);
                
                index_login.commit_transaction();
                commit_transaction();
                
                set_pkey(model, key);
            } catch (tokyo::Exception &ex) {
                index_login.abort_transaction();
                abort_transaction();
                throw ex;
            } catch (std::string &ex) {
                index_login.abort_transaction();
                abort_transaction();
                throw ex;
            }
        }
        
        virtual void remove(User *model) {
            if(model->pkey() != 0) {
                try {
                    begin_transaction();
                    index_login.begin_transaction();
                    
                    User u(model->pkey());
                    this->_remove(model->pkey());
                    for(std::list<std::string>::const_iterator iter = model->logins().begin();
                        iter != model->logins().end();
                        ++iter) {
                        index_login.remove(*iter, model->pkey());
                    }
                    search_name.remove(model->pkey());
                    search_email.remove(model->pkey());
                    
                    search_name.optimize();
                    search_email.optimize();
                    
                    index_login.commit_transaction();
                    commit_transaction();
                    
                    set_pkey(model, 0);
                } catch (tokyo::Exception &ex) {
                    index_login.abort_transaction();
                    abort_transaction();
                    throw ex;
                } catch (std::string &ex) {
                    index_login.abort_transaction();
                    abort_transaction();
                    throw ex;
                }
            }
        }
    };
    
    
    /**************************************************************************
     * User Lua Integration.
     *************************************************************************/
    
    int User_allowed(User *obj, lua_State *L) {
        lua_newtable(L);
        int i = 0;
        std::list<std::string> actions = obj->allowed();
        for(std::list<std::string>::const_iterator iter = actions.begin();
            iter != actions.end();
            ++iter) {
            lua_pushstring(L, iter->c_str());
            lua_rawseti(L, -2, ++i);
        }
        return 1;
    }

    int User_denied(User *obj, lua_State *L) {
        lua_newtable(L);
        int i = 0;
        std::list<std::string> actions = obj->denied();
        for(std::list<std::string>::const_iterator iter = actions.begin();
            iter != actions.end();
            ++iter) {
            lua_pushstring(L, iter->c_str());
            lua_rawseti(L, -2, ++i);
        }
        return 1;
    }
    
    int User_logins(User *obj, lua_State *L) {
        lua_newtable(L);
        int i = 0;
        std::list<std::string> logins = obj->logins();
        for(std::list<std::string>::const_iterator iter = logins.begin();
            iter != logins.end();
            ++iter) {
            lua_pushstring(L, iter->c_str());
            lua_rawseti(L, -2, ++i);
        }
        return 1;
    }
    
    int User_check_allowed(User *obj, lua_State *L) {
        const char *action = luaL_checkstring(L, -1);
        lua_pushboolean(L, obj->check_allowed(std::string(action)));
        return 1;
    }
    
    int User_role(User *obj, lua_State *L) {
        Lunar<Role>::push(L, &(obj->role()), false);
        return 1;
    }
};

const char User::LUNAR_CLASS_NAME[] = "Role";

Lunar<User>::RegType User::LUNAR_METHODS[] = {
LUNAR_STRING_GETTER(User, name),
LUNAR_STRING_GETTER(User, email),
LUNAR_INTEGER_GETTER(User, login_count, unsigned long long),
LUNAR_INTEGER_GETTER(User, pkey, unsigned long long),
LUNAR_STATIC_METHOD(User, allowed),
LUNAR_STATIC_METHOD(User, denied),
LUNAR_STATIC_METHOD(User, logins),
LUNAR_STATIC_METHOD(User, check_allowed),
LUNAR_STATIC_METHOD(User, role),
{0,0,0}
};


/******************************************************************************
 * User methods
 *****************************************************************************/

std::list<User *> User::all() {
    UserDB dao;
    std::list<User *> results;
    dao.all(results);
    return results;
}

std::list<User *> User::like(const std::string &term) {
    UserDB dao;
    std::set<unsigned long long> keys;
    dao.search_name.like(term, keys);
    dao.search_email.like(term, keys);
    
    std::list<User *> results;
    for(std::set<unsigned long long>::const_iterator iter = keys.begin();
        iter != keys.end();
        ++iter) {
        results.push_back(new User(*iter));
    }
    
    return results;
}

void User::at(unsigned long long key, User *model) {
    UserDB dao;
    dao.at(key, model);
}

void User::at_login(const std::string &login, User *model) {
    UserDB dao;
    
    std::set<unsigned long long> pkeys(dao.index_login.is(login));
    if(pkeys.size() == 0)
        throw std::string("Unknown User Login ").append(login).append(".");
    else if(pkeys.size() > 1)
        throw std::string("Ambiguous User Login ").append(login).append(".");
    
    dao.at(*(pkeys.begin()), model);
}

User::User() : _login_count(0) {
    _cached_allowed = NULL;
}

User::User(unsigned long long key) : _login_count(0) {
    _cached_allowed = NULL;
    User::at(key, this);
}

User::User(const std::string &login) : _login_count(0) {
    _cached_allowed = NULL;
    User::at_login(login, this);
}

User::User(lua_State *L) : _login_count(0) {
    _cached_allowed = NULL;
}

User::~User() {
    if(_cached_allowed)
        delete _cached_allowed;
   _cached_allowed = NULL;
}

void User::role(const Role &role) {
    if(_cached_allowed)
        delete _cached_allowed;
    _cached_allowed = NULL;
    _role = role;
}

namespace {
    std::string digest_string(const std::string &passwd) {
        char *buffer = (char *)calloc(passwd.size() * 8 + 1,
                                      sizeof(char));
        for(int h = 0; h < 8; ++h) {
            memcpy(buffer + (passwd.size() * h),
                   passwd.c_str(),
                   passwd.size());
        }
        
        crypto::SHA1 sha1;
        sha1.input(buffer,
                   passwd.size() * 8);
        free(buffer);
        
        unsigned digest[5];
        sha1.result(digest);
        buffer = (char *)calloc(41, sizeof(char));
        sprintf(buffer, "%x%X%x%X%x",
                digest[0],
                digest[1],
                digest[2],
                digest[3],
                digest[4]);
        
        std::string result(buffer);
        free(buffer);
        
        return result;
    }
};

void User::cookie(const std::string &cookie) {
    _cookie = digest_string(cookie);
}

bool User::check_cookie(const std::string &cookie) {
    return (_cookie.compare(digest_string(cookie)) == 0);
}

bool User::check_allowed(const std::string &action) {
    if(!_cached_allowed) {
        _cached_allowed = new std::set<std::string>();
        
        std::list<std::string> actions = role().allowed();
        _cached_allowed->insert(actions.begin(), actions.end());
        
        actions = allowed();
        _cached_allowed->insert(actions.begin(), actions.end());
        
        actions = denied();
        for(std::list<std::string>::const_iterator iter = actions.begin();
            iter != actions.end();
            ++iter)
            _cached_allowed->erase(*iter);
    }
    
    return (_cached_allowed->count(action) == 1);
}

const std::string User::serialize() const {
    std::ostringstream data;
    int h = 0, i = 0, j = 0;
    
    data << "name=\"" << escape(_name) << "\";\n";
    data << "cookie=\"" << escape(_cookie) << "\";\n";
    data << "email=\"" << escape(_email) << "\";\n";
    data << "count=\"" << _login_count << "\";\n";
    data << "role=\"" << _role.pkey() << "\";\n";
    data << "login{\n";
    for(std::list<std::string>::const_iterator iter = _logins.begin();
        iter != _logins.end();
        ++iter) {
        data << "    l" << h << "=\"" << escape(*iter) << "\";\n";
    }
    data << "};\n";
    data << "allow{\n";
    for(std::list<std::string>::const_iterator iter = _allowed.begin();
        iter != _allowed.end();
        ++iter, ++i) {
        data << "    a" << i << "=\"" << escape(*iter) << "\";\n";
    }
    data << "};\n";
    data << "deny{\n";
    for(std::list<std::string>::const_iterator iter = _denied.begin();
        iter != _denied.end();
        ++iter, ++j) {
        data << "    d" << j << "=\"" << escape(*iter) << "\";\n";
    }
    data << "};\n";
    
    return data.str();
}

void User::populate(OpenProp::File *props) {
    name(std::string(props->getValue("name")));
    _cookie = std::string(props->getValue("cookie"));
    email(std::string(props->getValue("email")));
    _login_count = (long)props->getValue("count");
    role(Role((long)props->getValue("role")));
    
    OpenProp::ElementIterator *iter = props->getElement("login")->getElements();
    while(iter->more())
        _logins.push_back(std::string(iter->next()->getValue()));
    iter = props->getElement("allow")->getElements();
    while(iter->more())
        _allowed.push_back(std::string(iter->next()->getValue()));
    iter = props->getElement("deny")->getElements();
    while(iter->more())
        _denied.push_back(std::string(iter->next()->getValue()));
}

ModelDB<User> *User::dao() const {
    return new UserDB();
}

