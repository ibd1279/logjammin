#include <string>
#include "lunar.h"

//! Rss Item class. Used to store an RSS item.
/*!
 \author Jason Watson
 \version 1.0
 \date July 30, 2009
 */
class RssItem {
    std::string _title, _link, _guid, _description, _author, _date;
public:
    //! Lua bindings classname.
    static const char LUNAR_CLASS_NAME[];
    
    //! Lua bindings method array.
    static Lunar<RssItem>::RegType LUNAR_METHODS[];
    
    //! Create a Rss Item object.
    RssItem() { };
    
    //! Lua constructor
    /*!
     \param L The lua state.
     */
    RssItem(lua_State *L) { };
    
    //! Get a copy of the title associated with this item
    /*!
     \return A copy of the title.
     */
    std::string title() const { return _title; };
    
    //! Set the title associated with this item.
    /*!
     \param s The item title.
     */
    void title(const std::string &s) { _title = s; };
    
    //! Get a copy of the link associated with this item.
    /*!
     \return A copy of the link.
     */
    std::string link() const { return _link.size() > 0 ? _link : _guid; };
    
    //! Set the link associated with this item.
    /*!
     \param s The item link.
     */
    void link(const std::string &s) { _link = s; };
    
    //! Set the guid associated with this item.
    /*!
     \param s The item guid.
     */
    void guid(const std::string &s) { _guid = s; };
    
    //! Get a copy of the description associated with this item.
    /*!
     \return A copy of the description.
     */
    std::string description() const { return _description; };
    
    //! Set the description associated with this item.
    /*!
     \param s The item description.
     */
    void description(const std::string &s) { _description = s; };
    
    //! Get a copy of the author associated with this item.
    /*!
     \return A copy of the author.
     */
    std::string author() const { return _author; };
    
    //! Set the author associated with this item.
    /*!
     \param s The item author.
     */
    void author(const std::string &s) { _author = s; };
    
    //! Get a copy of the date associated with this item.
    /*!
     \return A copy of the date.
     */
    std::string date() const { return _date; };
    
    //! Set the date associated with this item.
    /*!
     \param s The item date.
     */
    void date(const std::string &s) { _date = s; };
};
