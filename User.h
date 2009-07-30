#pragma once
#include <string>
#include <list>
#include <set>
#include "Model.h"
#include "lunar.h"
#include "Role.h"

//! User Class.
/*!
 \author Jason Watson
 \version 1.0
 \date July 29, 2009
 */
class User : public Model<User> {
public:
    //! Lua bindings classname.
    static const char LUNAR_CLASS_NAME[];
    
    //! Lua Bindings method array.
    static Lunar<User>::RegType LUNAR_METHODS[];
    
    /*******************************************************************
     * Static methods.
     ******************************************************************/
    
    //! Get a list of all users.
    /*!
     \par Users in the list must be deallocated with "delete".
     \return A list of all users.
     */
    static std::list<User *> all();
    
    //! Get a list of users matching a specific term.
    /*!
     \par The email address and name are searched.
     \par Users in the list must be deallocated with "delete".
     \param term The search term.
     \return A list of matching users.
     */
    static std::list<User *> like(const std::string &term);
    
    //! Get a user by primary key.
    /*!
     \param key The primary key.
     \param model Pointer to the object to populate.
     */
    static void at(unsigned long long key, User *model);
    
    //! Get a user by login.
    /*!
     \param login The login to search for.
     \param model Pointer to the object to populate.
     */
    static void at_login(const std::string &login, User *model);

    /*******************************************************************
     * ctor's and dtor's
     ******************************************************************/
    
    //! Create a user object.
    User();
    
    //! Load a user object by primary key.
    /*!
     \param key The primary key.
     \sa at()
     */
    User(unsigned long long key);
    
    //! Load a user object by login.
    /*!
     \param login The login.
     \sa at_login()
     */
    User(const std::string &login);
    
    //! Create a user object in a lua context.
    /*!
     \par This constructor creates an empty user object for placement in a
     Lua context.
     \param L The lua context.
     */
    User(lua_State *L);
    
    //! Delete the user object.
    virtual ~User();

    /*******************************************************************
     * instance methods.
     ******************************************************************/
    
    //! Get the name of the user.
    /*!
     \return A copy of the user name string.
     */
    std::string name() const { return _name; };
    
    //! Set the name of the user.
    /*!
     \param name The user name.
     */
    void name(const std::string &name) { _name = name; };
    
    //! Get the email address of the user.
    /*!
     \return A copy of the user email address.
     */
    std::string email() const { return _email; };
    
    //! Set the email address of the user.
    /*!
     \param email The user email address.
     */
    void email(const std::string &email) { _email = email; };
    
    //! Get the login count for the user.
    /*!
     \par This is used to prevent replay attacks on the user login.
     \return The number of times the user has logged in.
     */
    unsigned long long login_count() const { return _login_count; };
    
    //! Increment the login count for the user.
    /*!
     \par After a successful login, this method is called and the user is
     updated with the new count, and a new cookie hash.
     */
    void incr_login_count() { _login_count++; };
    
    //! Get the role for this user.
    /*!
     \return Reference to the user role.
     */
    Role &role() { return _role; };
    
    //! Get the role for this user.
    /*!
     \return Const reference to the user role.
     */
    const Role &role() const { return _role; };
    
    //! Set the role for this user.
    /*!
     \param role The user role.
     */
    void role(const Role &role);
    
    //! Get a reference to the user specific allowed actions.
    /*!
     \return Reference to the allowed actions.
     \sa denied() and check_allowed()
     */
    std::list<std::string> &allowed() { return _allowed; };
    
    //! Get a constant reference to the user specific allowed actions.
    /*!
     \return Const reference to the allowed actions.
     \sa denied() and check_allowed()
     */
    const std::list<std::string> &allowed() const { return _allowed; };
    
    //! Get a reference to the user specific denied actions.
    /*!
     \return Reference to the denied actions.
     \sa allowed() and check_allowed()
     */
    std::list<std::string> &denied() { return _denied; };

    //! Get a constant reference to the user specific denied actions.
    /*!
     \return Const reference to the denied actions.
     \sa allowed() and check_allowed()
     */
    const std::list<std::string> &denied() const { return _denied; };
    
    //! Get a reference to the list of logins.
    /*!
     \return Reference to the logins.
     */
    std::list<std::string> &logins() { return _logins; };

    //! Get a constant reference to the list of logins.
    /*!
     \return Const reference to the logins.
     */
    const std::list<std::string> &logins() const { return _logins; };
    
    
    //! Set the cookie value.
    /*!
     \par Sets the cookie value.  The value of the cookie is not actually stored.
     A SHA-1 digest of the message is computed and the digest is stored with the
     user.
     \param cookie String to create a cookie digest for.
     */
    void cookie(const std::string &cookie);

    //! Checks that the provided string matches the stored cookie.
    /*!
     \par Used to verify that a cookie value matches the stored cookie value.
     \param cookie String to compare to the stored cookie digest.
     \return true if the cookies match, false otherwise.
     */
    virtual bool check_cookie(const std::string &cookie);
    
    //! Check a user is allowed to invoke an action.
    /*!
     \par Actions allowed in the role and the user are flattened into a single
     set. The users specifically denied actions are removed from that set. This
     method looks to find an action in the set. If the action does not exist in
     the set, it returns false.
     \param action to check for.
     \return true if the user is allowed to invoke the action, false otherwise.
     */
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
