#include <string>
#include "lunar.h"

class RssItem {
    std::string _title, _link, _guid, _description, _author, _date;
public:
    //! Lua bindings classname.
    static const char LUNAR_CLASS_NAME[];
    
    //! Lua bindings method array.
    static Lunar<RssItem>::RegType LUNAR_METHODS[];
    
    RssItem() { };
    RssItem(lua_State *L) { };
    
    std::string title() const { return _title; };
    void title(const std::string &s) { _title = s; };
    std::string link() const { return _link.size() > 0 ? _link : _guid; };
    void link(const std::string &s) { _link = s; };
    void guid(const std::string &s) { _guid = s; };
    std::string description() const { return _description; };
    void description(const std::string &s) { _description = s; };
    std::string author() const { return _author; };
    void author(const std::string &s) { _author = s; };
    std::string date() const { return _date; };
    void date(const std::string &s) { _date = s; };
};
