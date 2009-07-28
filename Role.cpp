#include "Role.h"

namespace {
    /**************************************************************************
     * Role Database
     *************************************************************************/
    
    const char ROLE_DB[] = "/var/db/logjammin/role.tcb";
    const char ROLE_INDX_NAME[] = "/var/db/logjammin/role_name.tcb";
    
    class RoleDB : public ModelDB<Role> {
        static void open_db_file(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmpint64, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, ROLE_DB, mode);
        }
        static void open_indx_file_name(TCBDB *db, int mode) {
            tcbdbsetcmpfunc(db, tccmplexical, NULL);
            tcbdbtune(db, -1, -1, -1, -1, -1, BDBTLARGE | BDBTBZIP);
            tcbdbopen(db, ROLE_INDX_NAME, mode);
        }
    public:
        tokyo::Index<unsigned long long, std::string> index_name;
        
        RoleDB() :
        ModelDB<Role>(&open_db_file, BDBOREADER | BDBOWRITER | BDBOCREAT),
        index_name(&open_indx_file_name, BDBOREADER | BDBOWRITER | BDBOCREAT)
        {
        }

        virtual void put(Role *model) {
            try {
                begin_transaction();
                index_name.begin_transaction();
                
                // Clean up the index.
                if(model->pkey() != 0) {
                    Role r(model->pkey());
                    index_name.remove(r.name(), model->pkey());
                }
                
                // Make sure the name isn't used elsewhere.
                std::set<unsigned long long> tmp = index_name.is(model->name());
                if(tmp.size() != 0)
                    throw tokyo::Exception("Constraint error",
                                           "Name already exists in role database.");
                
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
        
        virtual void remove(Role *model) {
            if(model->pkey() != 0) {
                try {
                    begin_transaction();
                    index_name.begin_transaction();
                    
                    Role r(model->pkey());
                    this->_remove(model->pkey());
                    index_name.remove(r.name(), model->pkey());
                    
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
     * Role Lua Integration.
     *************************************************************************/

    int Role_allowed(Role *obj, lua_State *L) {
        lua_newtable(L);
        int i = 0;
        std::list<std::string> allowed = obj->allowed();
        for(std::list<std::string>::const_iterator iter = allowed.begin();
            iter != allowed.end();
            ++iter) {
            lua_pushstring(L, iter->c_str());
            lua_rawseti(L, -2, ++i);
        }
        return 1;
    }
    
};

const char Role::LUNAR_CLASS_NAME[] = "Role";

Lunar<Role>::RegType Role::LUNAR_METHODS[] = {
    LUNAR_STATIC_METHOD(Role, allowed),
    LUNAR_STRING_GETTER(Role, name),
    LUNAR_INTEGER_GETTER(Role, pkey, unsigned long long),
    {0,0,0}
};


/******************************************************************************
 * Role methods
 *****************************************************************************/

std::list<Role *> Role::all() {
    RoleDB dao;
    std::list<Role *> results;
    dao.all(results);
    return results;
}

void Role::at(unsigned long long key, Role *model) {
    RoleDB dao;
    dao.at(key, model);
}

void Role::at_name(const std::string &name, Role *model) {
    RoleDB dao;
    
    std::set<unsigned long long> pkeys(dao.index_name.is(name));
    if(pkeys.size() == 0)
        throw std::string("Unknown Role Name ").append(name).append(".");
    else if(pkeys.size() > 1)
        throw std::string("Ambiguous Role Name ").append(name).append(".");
    
    dao.at(*(pkeys.begin()), model);
}

Role::Role() {
}

Role::Role(unsigned long long key) {
    Role::at(key, this);
}

Role::Role(const std::string &name) {
    Role::at_name(name, this);
}

Role::Role(lua_State *L) {
}

Role::~Role() {
}

const std::string Role::serialize() const {
    std::ostringstream data;
    int i = 0;
    
    data << "name=\"" << escape(_name) << "\";\nallow{\n";
    for(std::list<std::string>::const_iterator iter = _allowed.begin();
        iter != _allowed.end();
        ++iter, ++i) {
        data << "    a" << i << "=\"" << escape(*iter) << "\";\n";
    }
    data << "};\n";
    
    return data.str();
}

void Role::populate(OpenProp::File *props) {
    name(std::string(props->getValue("name")));
    
    OpenProp::ElementIterator *iter = props->getElement("allow")->getElements();
    while(iter->more())
        _allowed.push_back(std::string(iter->next()->getValue()));    
}

ModelDB<Role> *Role::dao() const {
    return new RoleDB();
}
