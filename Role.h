#pragma once
#include <string>
#include <list>
#include "Model.h"
#include "lunar.h"

//! Role Class.
/*!
 \author Jason Watson
 \version 1.0
 \date July 9, 2009
 */
class Role : public Model<Role> {
public:
    //! Lua bindings class name.
    static const char LUNAR_CLASS_NAME[];
    
    //! Luna bindings method array.
    static Lunar<Role>::RegType LUNAR_METHODS[];
    
    //! Get a list of all roles in the database.
    /*!
     \return A list of roles.
     */
    static std::list<Role *> all();
    
    //! Get a role by primary key.
    /*!
     \param key the primary key.
     \param model Pointer to the object to populate.
     \exception tokyo::Exception When the record cannot be found.
     */
    static void at(unsigned long long key, Role *model);
    
    //! Get a role by name.
    /*!
     \param name The role name.
     \param model Pointer to the object to populate.
     \exception tokyo::Exception When the record cannot be read.
     \exception std::string When the record does not exist.
     */
    static void at_name(const std::string &name, Role *model);
    
    //! Create a new role object.
    Role();
    
    //! Create a new role object from a primary key.
    /*!
     \param key The primary key.
     \exception tokyo::Exception When the record cannot be found.
     */
    Role(unsigned long long key);
    
    //! Create a new role object from the name.
    /*!
     \param name The role name.
     \exception tokyo::Exception When the record cannot be read.
     \exception std::string When the record does not exist.
     */
    Role(const std::string &name);
    
    //! Lua constructor.
    /*!
     \param L Pointer to the lua state.
     */
    Role(lua_State *L);
    
    //! Delete the role.
    virtual ~Role();
    
    //! Get the name.
    /*!
     \return The name of the project.
     */
    std::string name() const { return _name; };
    
    //! Set the name.
    /*!
     \param name The name of the project.
     */
    void name(const std::string &name) { _name = name; };
    
    //! List of allowed actions.
    /*!
     \return A reference to the list of allowed actions.
     */
    std::list<std::string> &allowed() { return _allowed; };
    
    //! List of allowed actions.
    /*!
     \return A copy of the list of allowed actions.
     */
    std::list<std::string> allowed() const { return _allowed; };
    
    virtual const std::string serialize() const;
    virtual void populate(OpenProp::File *props);
protected:
    virtual ModelDB<Role> *dao() const;
private:
    std::string _name;
    std::list<std::string> _allowed;
};
