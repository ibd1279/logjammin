#pragma once
#include <set>
#include <list>
#include "Model.h"
#include "Project.h"
#include "User.h"
#include "lunar.h"

//! Backlog Class.
/*!
 \author Jason Watson
 \version 1.0
 \date July 3, 2009
 */
class Backlog : public Model<Backlog> {
public:
    //! Lua bindings classname.
    static const char LUNAR_CLASS_NAME[];
    
    //! Lua bindings method array.
    static Lunar<Backlog>::RegType LUNAR_METHODS[];
    
    //! Get all backlogs based on a natural key.
    /*!
     \param project The Project to search under.
     \param version The version to get the backlogs for.
     \param category The category to get the backlogs for.
     \return A list of backlogs
     */
    static std::list<Backlog *> all(const Project &project,
                                    const std::string &version,
                                    const std::string &category);
    
    //! Get a list of backlogs matching the provided search term.
    /*!
     \param term The term to search for.
     \param project The Project to search under.
     \param version The version to search under.
     \param category The category to search under.
     \return A list of backlogs
     */
    static std::list<Backlog *> like(const std::string &term,
                                     const Project &project,
                                     const std::string &version,
                                     const std::string &category);
    
    //! load a backlog object by primary key.
    /*!
     \param key The primary key.
     \param model Pointer to the object to populate.
     */
    static void at(const unsigned long long key, Backlog *model);
    
    //! Create a new Backlog object.
    Backlog();
    
    //! Load a backlog object by key.
    /*!
     \param key The primary key.
     */
    Backlog(unsigned long long key);
    
    //! Lua constructor.
    /*!
     \param L Pointer to a lua state.
     */    
    Backlog(lua_State *L);
    
    //! Delete the backlog object.
    virtual ~Backlog();
    
    //! Get the backlog entry name.
    /*!
     \return The name.
     */
    std::string brief() const { return _brief; };
    
    //! Get a reference to the project associated with this task.
    /*!
     \return A reference to the project associated with this task.
     */
    Project &project() { return _project; };
    
    //! Get a copy of the project associated with this task.
    /*!
     \return A copy of the project associated with this task.
     */
    const Project &project() const { return _project; };
    
    //! Set the project associated with this task.
    /*!
     \param project The project.
     */
    void project(const Project &project) { _project = project; };
    
    std::string version() const { return _version; };
    void version(const std::string &v) { _version = v; };
    std::string category() const { return _category; };
    void category(const std::string &c) { _category = c; };
    std::string story() const { return _story; };
    void story(const std::string &s);
    const User &user() const { return _user; };
    User &user() { return _user; };
    void user(const User &user) { _user = user; };
    std::string disposition() const { return _disposition; };
    void disposition(const std::string &s) { _disposition = s; };
    double estimate() const { return _estimate; };
    void estimate(const double e) { _estimate = e; };
    std::string natural_key() const;
    
    std::list<std::string> &comments() { return _comments; };
    std::list<std::string> comments() const { return _comments; };
    std::set<std::string> &tags() { return _tags; };
    std::set<std::string> tags() const { return _tags; };
    
    virtual const std::string serialize() const;
    virtual void populate(OpenProp::File *props);
protected:
    virtual ModelDB<Backlog> *dao() const;
private:
    std::string _brief, _version, _category, _story, _disposition;
    Project _project;
    User _user;
    std::list<std::string> _comments;
    std::set<std::string> _tags;
    double _estimate;
};
