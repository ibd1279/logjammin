/*
 \file ModuleCompilier.cpp
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

#include "ModuleCompilier.h"
#include <string>
#include <iostream>

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
