#pragma once
#include <string>
#include <list>
#include <set>
#include "Model.h"
#include "lunar.h"
#include "Role.h"

class User : public Model<User> {
public:
    static const char LUNAR_CLASS_NAME[];
    static Lunar<User>::RegType LUNAR_METHODS[];
    
    static std::list<User *> all();
    static std::list<User *> like(const std::string &term);
    static void at(unsigned long long key, User *model);
    static void at_login(const std::string &login, User *model);
    
    User();
    User(unsigned long long key);
    User(const std::string &login);
    User(lua_State *L);
    virtual ~User();
    
    std::string name() const { return _name; };
    void name(const std::string &name) { _name = name; };
    std::string email() const { return _email; };
    void email(const std::string &email) { _email = email; };
    unsigned long long login_count() const { return _login_count; };
    void incr_login_count() { _login_count++; };
    
    Role &role() { return _role; };
    const Role &role() const { return _role; };
    void role(const Role &role);
    std::list<std::string> &allowed() { return _allowed; };
    const std::list<std::string> &allowed() const { return _allowed; };
    std::list<std::string> &denied() { return _denied; };
    const std::list<std::string> &denied() const { return _denied; };
    std::list<std::string> &logins() { return _logins; };
    const std::list<std::string> &logins() const { return _logins; };
    
    void cookie(const std::string &cookie);
    virtual bool check_cookie(const std::string &cookie);
    virtual bool check_allowed(const std::string &action);
    
    virtual const std::string serialize() const;
    virtual void populate(OpenProp::File *props);
protected:
    virtual ModelDB<User> *dao() const;
private:
    std::string _name, _cookie, _email;
    unsigned long long _login_count;
    Role _role;
    std::list<std::string> _allowed, _denied, _logins;
    std::set<std::string> *_cached_allowed;
};
