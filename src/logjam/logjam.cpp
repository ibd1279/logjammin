/*
 \file logjam.cpp
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


#include "lj/Client.h"
#include "lj/Bson.h"
#include "lj/Logger.h"
#include "build/default/config.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#ifdef HAVE_EDITLINE
#include <histedit.h>
#endif

namespace {
    bool exit_line(const std::string& line)
    {
        if (std::string("quit\n").compare(line) == 0 ||
            std::string("exit\n").compare(line) == 0 ||
            std::string("\\q\n").compare(line) == 0)
        {
            return true;
        }
        return false;
    }
    
    bool send_line(const std::string& line)
    {
        if (std::string("\\send\n").compare(line) == 0 ||
            std::string("\\go\n").compare(line) == 0)
        {
            return true;
        }
        return false;
    }
    
    bool load_line(const std::string& line)
    {
        if (std::string("\\load ").compare(line.substr(0, 6)) == 0)
        {
            return true;
        }
        return false;
    }
    
    bool edit_line(const std::string& line)
    {
        if (std::string("\\edit\n").compare(line) == 0 ||
            std::string("\\e\n").compare(line) == 0)
        {
            return true;
        }
        return false;
    }
#ifdef HAVE_EDITLINE
    char *editline_prompt(EditLine *e) {
        return ">";
    }
    void input_loop(lj::Client* dispatch)
    {
        EditLine* el = el_init("logjam", stdin, stdout, stderr);
        History* hist = history_init();
        HistEvent ev;
        
        el_set(el, EL_PROMPT, &editline_prompt);
        el_set(el, EL_EDITOR, "vi");
        history(hist, &ev, H_SETSIZE, 100);
        el_set(el, EL_HIST, history, hist);
        
        std::string script;
        std::string old_script;
        while (true)
        {
            int sz;
            const char *line = el_gets(el, &sz);
            if (!line)
            {
                break;
            }
            if (sz && line && *line)
            {
                history(hist, &ev, H_ENTER, line);
            }
            else
            {
                continue;
            }
            
            if (edit_line(line))
            {
                char* ed = getenv("EDITOR");
                if (ed)
                {
                    std::string editor(ed);
                    char fn_tmp[L_tmpnam];
                    if (tmpnam(fn_tmp))
                    {
                        std::string fn(fn_tmp);
                        fn.append(".lua");
                        std::ofstream fo(fn.c_str());
                        if (script.size() < 1)
                        {
                            fo << old_script;
                        }
                        else
                        {
                            fo << script;
                        }
                        fo.close();
                        
                        editor.append(" ").append(fn);
                        system(editor.c_str());
                        
                        std::ifstream fi(fn.c_str());
                        char buf[1024];
                        script.clear();
                        while(fi.good())
                        {
                            fi.read(buf, 1024);
                            int h = fi.gcount();
                            script.append(buf, h);
                        }
                        fi.close();
                    }
                    else
                    {
                        std::cout << "Unable to make temp filename.";
                    }
                }
                else
                {
                    std::cout << "EDITOR must be set to use this feature.";
                }
            }
            else if (exit_line(line))
            {
                break;
            }
            else if (send_line(line))
            {
                lj::Bson* b = dispatch->send_command(script);
                old_script = script;
                script.clear();
                std::cout << lj::bson_as_pretty_string(*b) << std::endl;
            }
            else if (load_line(line))
            {
                // load file to send.
            }
            else
            {
                script.append(line);
            }
        }
        history_end(hist);
        el_end(el);
    }
#else
    void input_loop(lj::Client* dispatch, lj::Socket_selector& ss) {
        std::string script;
        while (std::cin.good())
        {
            std::string buffer;
            std::cout << ">" << std::flush;
            getline(std::cin, buffer);
            if (exit_line(buffer))
            {
                break;
            }
            else if (send_line(buffer))
            {
                lj::Bson* b = dispatch->send_command(script);
                old_script = script;
                script.clear();
                std::cout << lj::bson_as_pretty_string(*b) << std::endl;
            }
            else if (load_line(buffer))
            {
                // load file to send.
            }
            else
            {
                script.append(buffer);
            }
        }
    }
#endif
};

int main(int argc, char * const argv[]) {
    lj::Log::debug.disable();
    lj::Log::info.disable();
    
    lj::Client* sb = lj::Client::connect("127.0.0.1", 27754);
    
    input_loop(sb);
    
    return 0;
}