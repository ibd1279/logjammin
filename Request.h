#pragma once

#include <map>
#include <list>
#include <string>
#include "lunar.h"
#include "Response.h"

namespace CGI {
    class Request {
        friend class CGI::Response;
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<CGI::Request>::RegType LUNAR_METHODS[];
        typedef std::multimap<std::string, std::string> param_map;
        typedef std::map<std::string, std::string> attribute_map;
        
        Request();
        Request(lua_State *L);
        ~Request();
        bool is_post() const { return _is_post; };
        bool is_https() const { return _is_https; };
        std::string server_software() const { return _server_software; };
        std::string server_name() const { return _server_name; };
        std::string gateway_interface() const { return _gateway_interface; };
        std::string server_protocol() const { return _server_protocol; };
        std::string server_port() const { return _server_port; };
        std::string request_method() const { return _request_method; };
        std::string path_info() const { return _path_info; };
        std::list<std::string> split_path_info() const { return _split_path_info; };
        std::string path_translated() const { return _path_translated; };
        std::string script_name() const { return _script_name; };
        std::string query_string() const { return _query_string; };
        std::string remote_host() const { return _remote_host; };
        std::string remote_addr() const { return _remote_addr; };
        std::string auth_type() const { return _auth_type; };
        std::string remote_user() const { return _remote_user; };
        std::string remote_ident() const { return _remote_ident; };
        std::string content_type() const { return _content_type; };
        unsigned long content_length() const { return _content_length; };
        std::string header(const std::string &) const;
        std::string original_request_host() const;
        std::string original_request_script() const;
        std::string original_request_file() const;
        std::string original_request() const;
        const param_map &params() const { return _params; };
        std::string param(const std::string &key) const {
            param_map::const_iterator iter;
            return (iter = _params.find(key)) != _params.end() ? iter->second : std::string("");
        };
        const bool has_param(const std::string &key) const {
            param_map::const_iterator iter;
            return ((iter = _params.find(key)) != _params.end());
        };
        const param_map &cookies() const { return _cookies; };
        std::string cookie(const std::string &key) const {
            param_map::const_iterator iter;
            return (iter = _cookies.find(key)) != _cookies.end() ? iter->second : std::string("");
        }
        const bool has_cookie(const std::string &key) const {
            param_map::const_iterator iter;
            return ((iter = _cookies.find(key)) != _cookies.end());
        }
        const attribute_map &attributes() const { return _attributes; };
        std::string attribute(const std::string &key) const {
            attribute_map::const_iterator iter;
            return (iter = _attributes.find(key)) != _attributes.end() ? iter->second : std::string("");
        }
        const bool has_attribute(const std::string key) const {
            attribute_map::const_iterator iter;
            return ((iter = _attributes.find(key)) != _attributes.end());
        }
        void attribute(const std::string &key, const std::string &value) {
            _attributes.insert(attribute_map::value_type(key, value));
        }
        template <class T>
        void context_object(const std::string &key, T *value, bool gc) {            
            Lunar<T>::push(_lua_state, value, gc);
            lua_setfield(_lua_state, LUA_GLOBALSINDEX, key.c_str());
        }
        template <class T>
        void context_object_list(const std::string &key, const std::list<T *> &l, bool gc) {
            lua_newtable(_lua_state);
            int i = 0;
            typename std::list<T *>::const_iterator iter;
            for(iter = l.begin(); iter != l.end(); ++iter) {
                Lunar<T>::push(_lua_state, *iter, gc);
                lua_rawseti(_lua_state, -2, ++i);
            }
            lua_setfield(_lua_state, LUA_GLOBALSINDEX, key.c_str());
        }
        template <class T>
        void context_object_map(const std::string &key, const std::map<std::string, T *> &m, bool gc) {
            lua_newtable(_lua_state);
            typename std::list<T *>::const_iterator iter;
            for(iter = m.begin(); iter != m.end(); ++iter) {
                lua_pushstring(_lua_state, (*iter).first.c_str());
                Lunar<T>::push(_lua_state, (*iter).second, gc);
                lua_settable(_lua_state, -3);
            }
            lua_setfield(_lua_state, LUA_GLOBALSINDEX, key.c_str());
        }
    private:
        void populate_cgi_parameters();
        std::string _server_software, _server_name, _gateway_interface;
        std::string _server_protocol, _server_port, _request_method, _path_info;
        std::string _path_translated, _script_name, _query_string, _remote_host;
        std::string _remote_addr, _auth_type, _remote_user, _remote_ident;
        std::string _content_type;
        std::list<std::string> _split_path_info;
        param_map _params, _cookies;
        attribute_map _attributes;
        unsigned long _content_length;
        bool _is_post, _is_https;
        lua_State *_lua_state;
    };
};