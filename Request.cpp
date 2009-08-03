#include <iostream>
#include "lua.hpp"
#include "lunar.h"
#include "Request.h"
#include "Response.h"
#include "Role.h"
#include "User.h"
#include "Project.h"
#include "Backlog.h"
#include "RssItem.h"

using std::multimap;
using std::string;
using std::pair;

// Hiding lua methods from public scope.
namespace {
    int Request_is_https(CGI::Request *obj, lua_State *L) {
        lua_pushboolean(L, obj->is_https());
        return 1;
    }
    
    int Request_is_post(CGI::Request *obj, lua_State *L) {
        lua_pushboolean(L, obj->is_post());
        return 1;
    }
    
    int Request_header(CGI::Request *obj, lua_State *L) {
        const char *key = luaL_checkstring(L, -1);
        lua_pushstring(L, obj->header(std::string(key)).c_str());
        return 1;
    }
    
    int Request_params(CGI::Request *obj, lua_State *L) {
        lua_newtable(L);
        CGI::Request::param_map params = obj->params();
        for(CGI::Request::param_map::const_iterator iter = params.begin();
            iter != params.end();
            ++iter) {
            lua_pushstring(L, iter->first.c_str());
            lua_pushstring(L, iter->second.c_str());
            lua_settable(L, -3);
        }
        return 1;
    }
    
    int Request_param(CGI::Request *obj, lua_State *L) {
        const char *key = luaL_checkstring(L, -1);
        pair<CGI::Request::param_map::const_iterator, CGI::Request::param_map::const_iterator> range =
            obj->params().equal_range(std::string(key));
        int h = 0;
        for(CGI::Request::param_map::const_iterator iter = range.first; iter != range.second; ++iter, ++h) {
            lua_pushstring(L, iter->second.c_str());
        }
        return h;
    }
    
    int Request_has_param(CGI::Request *obj, lua_State *L) {
        const char *key = luaL_checkstring(L, -1);
        CGI::Request::param_map params = obj->params();
        lua_pushboolean(L, params.find(std::string(key)) != params.end());
        return 1;
    }
    
    int Request_cookies(CGI::Request *obj, lua_State *L) {
        lua_newtable(L);
        CGI::Request::param_map cookies = obj->cookies();
        for(CGI::Request::param_map::const_iterator iter = cookies.begin();
            iter != cookies.end();
            ++iter) {
            lua_pushstring(L, iter->first.c_str());
            lua_pushstring(L, iter->second.c_str());
            lua_settable(L, -3);
        }
        return 1;
    }
    
    int Request_cookie(CGI::Request *obj, lua_State *L) {
        const char *key = luaL_checkstring(L, -1);
        pair<CGI::Request::param_map::const_iterator, CGI::Request::param_map::const_iterator> range =
            obj->cookies().equal_range(std::string(key));
        int h = 0;
        for(CGI::Request::param_map::const_iterator iter = range.first; iter != range.second; ++iter, ++h) {
            lua_pushstring(L, iter->second.c_str());
        }
        return h;
    }
    
    int Request_has_cookie(CGI::Request *obj, lua_State *L) {
        const char *key = luaL_checkstring(L, -1);
        CGI::Request::param_map cookies = obj->cookies();
        lua_pushboolean(L, cookies.find(std::string(key)) != cookies.end());
        return 1;
    }
    
    int Request_attributes(CGI::Request *obj, lua_State *L) {
        lua_newtable(L);
        CGI::Request::attribute_map attributes = obj->attributes();
        for(CGI::Request::attribute_map::const_iterator iter = attributes.begin();
            iter != attributes.end();
            ++iter) {
            lua_pushstring(L, iter->first.c_str());
            lua_pushstring(L, iter->second.c_str());
            lua_settable(L, -3);
        }
        return 1;
    }
    
    int Request_attribute(CGI::Request *obj, lua_State *L) {
        const char *key = luaL_checkstring(L, -1);
        pair<CGI::Request::attribute_map::const_iterator, CGI::Request::attribute_map::const_iterator> range =
            obj->attributes().equal_range(std::string(key));
        int h = 0;
        for(CGI::Request::attribute_map::const_iterator iter = range.first; iter != range.second; ++iter, ++h) {
            lua_pushstring(L, iter->second.c_str());
        }
        return h;
    }
    
    int Request_has_attribute(CGI::Request *obj, lua_State *L) {
        const char *key = luaL_checkstring(L, -1);
        CGI::Request::attribute_map attributes = obj->attributes();
        lua_pushboolean(L, attributes.find(std::string(key)) != attributes.end());
        return 1;
    }
};

const char CGI::Request::LUNAR_CLASS_NAME[] = "Request";

Lunar<CGI::Request>::RegType CGI::Request::LUNAR_METHODS[] = {
LUNAR_STATIC_METHOD(Request, is_https),
LUNAR_STATIC_METHOD(Request, is_post),
LUNAR_STRING_GETTER(Request, server_software),
LUNAR_STRING_GETTER(Request, server_name),
LUNAR_STRING_GETTER(Request, gateway_interface),
LUNAR_STRING_GETTER(Request, server_protocol),
LUNAR_STRING_GETTER(Request, server_port),
LUNAR_STRING_GETTER(Request, request_method),
LUNAR_STRING_GETTER(Request, path_info),
LUNAR_STRING_GETTER(Request, path_translated),        
LUNAR_STRING_GETTER(Request, script_name),
LUNAR_STRING_GETTER(Request, query_string),
LUNAR_STRING_GETTER(Request, remote_host),
LUNAR_STRING_GETTER(Request, remote_addr),
LUNAR_STRING_GETTER(Request, auth_type),
LUNAR_STRING_GETTER(Request, remote_user),
LUNAR_STRING_GETTER(Request, remote_ident),
LUNAR_STRING_GETTER(Request, content_type),
LUNAR_INTEGER_GETTER(Request, content_length, unsigned long),
LUNAR_STRING_GETTER(Request, original_request),
LUNAR_STRING_GETTER(Request, original_request_file),
LUNAR_STRING_GETTER(Request, original_request_script),
LUNAR_STRING_GETTER(Request, original_request_host),
LUNAR_STATIC_METHOD(Request, header),
LUNAR_STATIC_METHOD(Request, params),
LUNAR_STATIC_METHOD(Request, param),
LUNAR_STATIC_METHOD(Request, has_param),
LUNAR_STATIC_METHOD(Request, cookies),
LUNAR_STATIC_METHOD(Request, cookie),
LUNAR_STATIC_METHOD(Request, has_cookie),
LUNAR_STATIC_METHOD(Request, attributes),
LUNAR_STATIC_METHOD(Request, attribute),
LUNAR_STATIC_METHOD(Request, has_attribute),
{0,0,0}
};

// Hiding internal CGI methods.
namespace  {
    string url_decode(const string &tmp) {
        string result;
        string::const_iterator iter = tmp.begin();
        for(; iter != tmp.end(); ++iter) {
            char c = *iter;
            if(c == '+')
                c = ' ';
            else if(c == '%') {
                const char hex[3] = {*++iter, *++iter, '\0'};
                c = (char)strtol(hex, NULL, 16);
                if(!c) c = '?';
            }
            if(c == '\r') continue;
            result.push_back(c);
        }
        return result;
    }
    void parse_params(const string &tmp, multimap<string, string> &params) {
        string key, value;
        bool both = false;
        for(string::const_iterator iter = tmp.begin(); iter != tmp.end(); ++iter) {
            if(*iter == '&') {
                params.insert(pair<string, string>(url_decode(key), url_decode(value)));
                key.erase();
                value.erase();
                both = false;
            } else if(*iter == '=') {
                both = true;
            
            } else {
                if(both)
                    value.push_back(*iter);
                else
                    key.push_back(*iter);
            }
        }
        params.insert(pair<string, string>(url_decode(key), url_decode(value)));
    }
    void parse_cookies(const string &tmp, multimap<string, string> &params) {
        string key, value;
        bool both = false, skip_space = false;
        for(string::const_iterator iter = tmp.begin(); iter != tmp.end(); ++iter) {
            if(skip_space && *iter == ' ') {
                skip_space = false;
            } else if(*iter == ';') {
                params.insert(pair<string, string>(key, value));
                key.erase();
                value.erase();
                both = false;
                skip_space = true;
            } else if(*iter == '=') {
                both = true;
                skip_space = false;
            } else {
                if(both)
                    value.push_back(*iter);
                else
                    key.push_back(*iter);
                skip_space = false;
            }
        }
        params.insert(pair<string, string>(key, value));
    }
    void parse_path_info(const std::string &path_info, std::list<std::string> &result) {
        std::string val;
        std::string::const_iterator iter = path_info.begin();
        for(; iter != path_info.end(); ++iter) {
            if(*iter == '/') {
                if(iter != path_info.begin()) {
                    result.push_back(val);
                }
                val.erase();
                continue;
            } else if(*iter == '\\') {
                // Backslash escapes next char, helpful for "slash" and "backslash".
                ++iter;
                if(iter == path_info.end())
                    break;
            }
            val.push_back(*iter);
        }
        if(val.size() > 0)
            result.push_back(val);
    }
    
    void html_escape_string(const char *str, std::string &result) {
    };
    
    int json_escape(lua_State *L) {
        std::string result;
        
        size_t sz;
        const char *str = lua_tolstring(L, 1, &sz);
        for(int h = 0; h < sz; ++h) {
            switch(str[h]) {
                case '\"':
                    result.append("\\\"");
                    break;
                case '\\':
                    result.append("\\\\");
                    break;
                case '/':
                    result.append("\\/");
                    break;
                case '\b':
                    result.append("\\b");
                    break;
                case '\f':
                    result.append("\\f");
                    break;
                case '\n':
                    result.append("\\n");
                    break;
                case '\r':
                    result.append("\\r");
                    break;
                case '\t':
                    result.append("\\t");
                    break;
                default:
                    result.push_back(str[h]);
            }
        }
        
        lua_pushstring(L, result.c_str());
        return 1;
    }
    
    int html_escape(lua_State *L) {
        std::string result;

        size_t sz;
        const char *str = lua_tolstring(L, 1, &sz);
        for(int h = 0; h < sz; ++h) {
            switch(str[h]) {
                case '"':
                    result.append("&quot;");
                    break;
                case '\'':
                    result.append("&apos;");
                    break;
                case '>':
                    result.append("&gt;");
                    break;
                case '<':
                    result.append("&lt;");
                    break;
                case '&':
                    result.append("&amp;");
                    break;
                case '|':
                    result.append("&brvbar;");
                    break;
                default:
                    result.push_back(str[h]);
            }
        }
        
        lua_pushstring(L, result.c_str());
        return 1;
    }    
};

CGI::Request::Request() {
    populate_cgi_parameters();
    
    _lua_state = lua_open();
    luaL_openlibs(_lua_state);    
    Lunar<Request>::Register(_lua_state);
    Lunar<Response>::Register(_lua_state);
    Lunar<Role>::Register(_lua_state);
    Lunar<User>::Register(_lua_state);
    Lunar<Project>::Register(_lua_state);
    Lunar<Backlog>::Register(_lua_state);
    Lunar<RssItem>::Register(_lua_state);
    
    lua_pushcfunction(_lua_state, html_escape);
    lua_setglobal(_lua_state, "_html");
    lua_pushcfunction(_lua_state, json_escape);
    lua_setglobal(_lua_state, "_json");
}

CGI::Request::Request(lua_State *L) {
    throw std::string("Cannot be used with a lua state.");
}

CGI::Request::~Request() {
    if(_lua_state)
        lua_close(_lua_state);
}

std::string CGI::Request::header(const std::string &h) const {
    const char *v = getenv(h.c_str());
    return v == NULL ? std::string("") : std::string(v);
}

//extern char **environ;

void CGI::Request::populate_cgi_parameters() {
    _is_post = false;
    _is_https = false;
    _server_software = header("SERVER_SOFTWARE");
    _server_name = header("SERVER_NAME");
    _gateway_interface = header("GATEWAY_INTERFACE");
    _server_protocol = header("SERVER_PROTOCOL");
    _server_port = header("SERVER_PORT");
    _request_method = header("REQUEST_METHOD");
    _path_info = header("PATH_INFO");
    _path_translated = header("PATH_TRANSLATED");
    _script_name = header("SCRIPT_NAME");
    _query_string = header("QUERY_STRING");
    _remote_host = header("REMOTE_HOST");
    _remote_addr = header("REMOTE_ADDR");
    _auth_type = header("AUTH_TYPE");
    _remote_user = header("REMOTE_USER");
    _remote_ident = header("REMOTE_IDENT");
    _content_type = header("CONTENT_TYPE");
        
    if(getenv("CONTENT_LENGTH") != NULL)
        _content_length = atoi(getenv("CONTENT_LENGTH"));
    
    if(getenv("HTTPS") != NULL)
        _is_https = true;
    
    if(_request_method.compare("POST") == 0) {
        _is_post = true;
        if(_content_length != 0 && _content_length < 2097152) {
            char *buffer = (char *)calloc((_content_length + 1), sizeof(char));
            std::cin.read(buffer, _content_length);
            parse_params(std::string(buffer), _params);
            free(buffer);
        }
    }
    
    if(getenv("HTTP_COOKIE") != NULL) {
        parse_cookies(std::string(getenv("HTTP_COOKIE")), _cookies);
    }
    
    parse_path_info(_path_info, _split_path_info);
    parse_params(_query_string, _params);
    
    /*
    char **tmp = environ;
    while(*tmp) {
        std::cerr << "env var " << *tmp << std::endl;
        tmp++;
    }*/
}

std::string CGI::Request::original_request_host() const {
    // Protocol
    std::string result("http");
    if(is_https())
        result.push_back('s');
    result.append("://");
    
    // Hostname
    result.append(server_name());
    
    // Port
    if(is_https() && server_port().compare("443") != 0)
        result.append(":").append(server_port());
    if(!is_https() && server_port().compare("80") != 0)
        result.append(":").append(server_port());
    
    return result;
}

std::string CGI::Request::original_request_script() const {
    std::string result(original_request_host());
    result.append(script_name());
    return result;
}

std::string CGI::Request::original_request_file() const {
    std::string result(original_request_script());
    result.append(path_info());
    return result;
}

std::string CGI::Request::original_request() const {
    std::string result(original_request_file());
    if(query_string().size() > 0) {
        result.push_back('?');
        result.append(query_string());
    }
    return result;
}

