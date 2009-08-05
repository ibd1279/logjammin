/*
 \file Response.cpp
 \author Jason Watson
 Copyright (c) 2009, Jason Watson
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 * Neither the name of the LogJammin nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include <fstream>
#include <sstream>
#include <iostream>
#include <list>
#include "Tokyo.h"
#include "ModuleCompilier.h"
#include "Response.h"
#include "Request.h"

using std::multimap;
using std::string;
using std::pair;
using std::list;

const char CGI::Response::LUNAR_CLASS_NAME[] = "Response";
Lunar<CGI::Response>::RegType CGI::Response::LUNAR_METHODS[] = {
LUNAR_MEMBER_METHOD(Response, write),
LUNAR_MEMBER_METHOD(Response, execute),
{0, 0, 0}
};

std::string CGI::Response::percent_encode(const std::string &input, bool spaces_as_plus) {
    std::ostringstream result;
    
    for(string::const_iterator iter(input.begin()); iter != input.end(); ++iter) {
        switch(*iter) {
            case ' ':
                if(spaces_as_plus) {
                    result << "+";
                    break;
                }
            case '\r':
            case '\n':
            case '*':
            case '\"':
            case '\'':
            case '(':
            case ')':
            case ';':
            case ':':
            case '@':
            case '&':
            case '=':
            case '+':
            case '$':
            case ',':
            case '/':
            case '?':
            case '%':
            case '[':
            case ']':
                char buffer[3];
                sprintf(buffer, "%02x", *iter);
                result << "%" << buffer;
                break;
            default:
                result << *iter;
        }
    }
    
    return result.str();
}

CGI::Response::Response() {
    content_type("text/html");
    _is_closed = false;
}

CGI::Response::Response(lua_State *L) {
    throw std::string("Cannot be used with a lua state.");
}

CGI::Response::~Response() {
}

void CGI::Response::redirect(const std::string &location, Request *request) {
    std::string target;
    if(request && location.size() > 0) {
        if(location.at(0) == '?') {
            // Same file, new query string.
            target = request->original_request_file();
            target.append(location);
        } else if(location.at(0) == '/') {
            // absolute file.
            target = request->original_request_host();
            target.append(location);
        } else {
            // doesn't start with a special character.
            target.append(location);
        }
    } else {
        target.append(location);
    }
    
    header("Status", "302 Moved");
    header("Location", target);
    _buffer = "";
    close();
}

void CGI::Response::status(int sc) {
    std::ostringstream data;
    data << sc;
    header("Status", data.str());
}

void CGI::Response::header(const std::string &h, const std::string &val, const bool replace) {
    if(replace) {
        pair<header_map::iterator, header_map::iterator> p = _headers.equal_range(h);
        _headers.erase(p.first, p.second);
    }
    _headers.insert(pair<std::string, std::string>(h, val));
}

void CGI::Response::cookie(const std::string &name, const std::string &value,
                           const std::string &path,
                           long long max_age, bool discard) {
    std::ostringstream cookie1, cookie2;
    cookie2 << name << "=" << value;
    if(discard)
        cookie2 << "; Discard";
    if(max_age > 0)
        cookie2 << "; Max-Age=" << max_age;
    cookie2 << "; Version=1";
    if(path.size() > 0)
        cookie2 << "; Path=" << path;
    
    cookie1 << name << "=" << value;
    if(max_age == 0) 
        cookie1 << "; expires=Fri, 13-Feb-2009 23:31:30 GMT";
    if(path.size() > 0)
        cookie1 << "; path=" << path;
    
    header("Set-Cookie", cookie1.str(), false);
    header("Set-Cookie2", cookie2.str(), false);
}

void CGI::Response::write(const std::string &s) {
    _buffer.append(s);
}

int CGI::Response::write(lua_State *L) {
    std::ostringstream data;
    while(lua_gettop(L)) {
        size_t len = 0;
        const char *str;
        switch(lua_type(L, 1)) {
            case LUA_TSTRING:
                str = lua_tolstring(L, 1, &len);
                data << std::string(str, len);
                break;
            case LUA_TNUMBER:
                data << lua_tointeger(L, 1);
                break;
            case LUA_TNIL:
                data << "nil";
                break;
            case LUA_TBOOLEAN:
                data << (lua_toboolean(L, 1) ? "true" : "false");
                break;
            case LUA_TTABLE:
                data << "[TABLE]";
                break;
            case LUA_TFUNCTION:
                data << "[FUNCTION]";
                break;
            case LUA_TTHREAD:
                data << "[THREAD]";
                break;
            case LUA_TUSERDATA:
                data << "[USER DATA]";
                break;
            case LUA_TLIGHTUSERDATA:
                data << "[LIGHT USER DATA]";
                break;
            case LUA_TNONE:
                data << "[NONE]";
                break;
            default:
                break;
        }
        lua_remove(L, 1);
    }
    write(data.str());
    return 0;
}

void CGI::Response::execute(const std::string &t, Request *request) {
    if(_is_closed) return;
    
    std::string fname = "/var/db/logjammin/";
    fname.append(t);
    
    std::ifstream file;
    file.open(fname.c_str());
    if (!file) {
        _buffer = std::string("Unable to open ").append(t).append(" for response. \n");
        std::cerr << "Unable to open " << t << " for response." << std::endl;
        status(500);
        close();
        return;
    }
    
    std::string script;
    try {
        ModuleCompilier compilier(file);
        script = compilier.script();
        const char* error = 0;
        
        if(luaL_dostring(request->_lua_state, script.c_str())) {
            error = lua_tostring(request->_lua_state, -1);
        }
        
        if(error) throw error;
    } catch (const char *str) {
        std::ostringstream data;
        data << "<h2>ERROR</h2><div>\n" << str << "\n</div>\n<pre>1:";
        int h = 1;
        for(std::string::const_iterator iter = script.begin(); iter != script.end(); ++iter) {
            switch (*iter) {
                case '\n':
                    data << *iter << ++h << ":";
                    break;
                case '<':
                    data << "&lt;";
                    break;
                case '>':
                    data << "&gt;";
                    break;
                case '&':
                    data << "&amp;";
                    break;
                default:
                    data << *iter;
                    break;
            }
        }
        data << "</pre>\n";
        _buffer = data.str();
        std::cerr << _buffer << std::endl;
        status(500);
        close();
    } catch (const std::string &str) {
        std::ostringstream data;
        data << "<h2>ERROR</h2><div>\n" << str << "\n</div>\n<pre>1:";
        int h = 1;
        for(std::string::const_iterator iter = script.begin(); iter != script.end(); ++iter) {
            switch (*iter) {
                case '\n':
                    data << *iter << ++h << ":";
                    break;
                case '<':
                    data << "&lt;";
                    break;
                case '>':
                    data << "&gt;";
                    break;
                case '&':
                    data << "&amp;";
                    break;
                default:
                    data << *iter;
                    break;
            }
        }
        data << "</pre>\n";
        _buffer = data.str();
        std::cerr << _buffer << std::endl;
        status(500);
        close();
    } catch (tokyo::Exception &ex) {
        std::ostringstream data;
        data << "<h2>ERROR</h2><div>\n" << ex.msg << "\n</div>\n<pre>1:";
        int h = 1;
        for(std::string::const_iterator iter = script.begin(); iter != script.end(); ++iter) {
            switch (*iter) {
                case '\n':
                    data << *iter << ++h << ":";
                    break;
                case '<':
                    data << "&lt;";
                    break;
                case '>':
                    data << "&gt;";
                    break;
                case '&':
                    data << "&amp;";
                    break;
                default:
                    data << *iter;
                    break;
            }
        }
        data << "</pre>\n";
        _buffer = data.str();
        std::cerr << _buffer << std::endl;
        status(500);
        close();
    }
    
    file.close();
}

int CGI::Response::execute(lua_State *L) {
    const char *file = luaL_checkstring(L, -1);
    lua_getfield(L, LUA_GLOBALSINDEX, "request");
    Request *request = Lunar<Request>::check(L, -1);
    execute(std::string(file), request);
    return 0;
}

void CGI::Response::close() {
    if(_is_closed) return;
    header_map::const_iterator iter;
    for(iter = _headers.begin(); iter != _headers.end(); ++iter) {
        std::cout << iter->first << ": " << iter->second << "\r\n";
    }
    std::cout << "\r\n" << _buffer;
    _is_closed = true;
}
