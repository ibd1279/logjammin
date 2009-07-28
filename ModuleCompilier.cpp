#include "ModuleCompilier.hpp"
#include <string>
#include <iostream>

namespace llamativo {

ModuleCompilier::ModuleCompilier(std::istream &is) : _script("response:write([[\n") {
    char prev[2] = {0, 0}, c;
    bool subst = false, code = false, append = true;
    while(true) {
        append = true;
        is.get(c);
        if(is.fail()) break;
        if(c == '{' && !subst && !code) {
            if(prev[0] == '$') {
                if(prev[1] == '\\') {
                    _script.erase(_script.end() - 2);
                } else {
                    _script.erase(_script.end() - 1);
                    _script.append("]])\nresponse:write(");
                    subst = true;
                    append = false;
                }
            }
        } else if(c == '}' && subst && !code) {
            _script.append(")\nresponse:write([[\n");
            subst = false;
            append = false;
        } else if(c == '[' && !subst && !code) {
            if(prev[0] == '[') {
                _script.erase(_script.end() - 1);
                _script.append("]])\nresponse:write('[[')\nresponse:write([[\n");
                c = 0;
                append = false;
            }
        } else if(c == ']' && !subst && !code) {
            if(prev[0] == ']') {
                _script.erase(_script.end() - 1);
                _script.append("]])\nresponse:write(']]')\nresponse:write([[\n");
                c = 0;
                append = false;
            }
        } else if(c == '?' && !subst && !code) {
            if(prev[0] == '<') {
                char buffer[3];
                is.read(buffer, 3);
                if(is.gcount() == 3) {
                    if(buffer[0] == 'l' && buffer[1] == 'c' && 
                            (buffer[2] == ' ' || buffer[2] == '\n' || buffer[2] == '\r')) {
                        _script.erase(_script.end() - 1);
                        _script.append("]])\n");
                        code = true;
                    } else {
                        _script.append(1, c);
                        _script.append(buffer, 3);
                    }
                } else {
                    _script.append(1, c);
                    _script.append(buffer, is.gcount());
                }
                append = false;
            }
        } else if(c == '?' && !subst && code) {
            is.get(c);
            if(is.fail()) break;
            if(c == '>') {
                _script.append("\nresponse:write([[\n");
                append = false;
                code = false;
            } else {
                _script.append(1, '?');
            }
        }

        if(append) _script.append(1, c);
        prev[1] = prev[0];
        prev[0] = c;
    }
    _script.append("]])\n");
}

ModuleCompilier::~ModuleCompilier() {
}

std::string& ModuleCompilier::script() {
    return _script;
}

}; // namespace llamativo
