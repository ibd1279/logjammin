#pragma once

#include <map>
#include <list>
#include <string>
#include "lunar.h"

namespace CGI {
    class Request;
    
    class Response {
    public:
        static const char LUNAR_CLASS_NAME[];
        static Lunar<Response>::RegType LUNAR_METHODS[];
        
        typedef std::multimap<std::string, std::string> header_map;
        
        Response();
        Response(lua_State *L);
        ~Response();
        
        bool is_closed() { return _is_closed; };
        
        void redirect(const std::string &, Request * = NULL);
        void header(const std::string &, const std::string &, const bool replace = true);
        void content_type(const std::string &v) { header("Content-Type", v); };
        void status(int);
        void cookie(const std::string &name, const std::string &value, long long max_age = -1, bool discard = false);
                
        void write(const std::string &);
        int write(lua_State *);
        void execute(const std::string &, Request *);
        int execute(lua_State *);
        void close();
    private:
        bool _is_closed;
        std::string _buffer;
        header_map _headers;
    };
    
};