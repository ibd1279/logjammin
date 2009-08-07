/*
 \file RssController.cpp
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

extern "C" {
#include <curl/curl.h>
#include <expat.h>
}
#include <list>
#include "Project.h"
#include "Backlog.h"
#include "User.h"
#include "RssItem.h"
#include "RssController.h"

namespace logjammin {
    namespace controller {
        bool CommitFeedController::is_requested(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            if(!request->has_attribute("authenticated"))
                return false;
            if(request->has_attribute("handled"))
                return false;
            
            if(args.size() != 2)
                return false;
            return (args.back().compare("commit-feed") == 0);
        }
        
        namespace {
            class RssParser {
                std::string line;
                bool in_item;
                long long min_date;
                signed short depth;
            public:
                std::list<RssItem *> items;
                RssParser(long long ts) : in_item(false), min_date(ts), depth(0) { };
                static void start_element(void *user_data,
                                          const XML_Char *tag_name,
                                          const XML_Char **atts) {
                    RssParser *state = static_cast<RssParser *>(user_data);
                    
                    std::string name(tag_name);
                    if(!state->in_item && name.compare("item") == 0) {
                        state->items.push_front(new RssItem());
                        state->in_item = true;
                        state->depth = 0;
                    } else if(state->in_item) {
                        if(state->depth == 0 && name.compare("title") == 0)
                            state->line.clear();
                        else if(state->depth == 0 && name.compare("link") == 0)
                            state->line.clear();
                        else if(state->depth == 0 && name.compare("description") == 0)
                            state->line.clear();
                        else if(state->depth == 0 && name.compare("author") == 0)
                            state->line.clear();
                        else if(state->depth == 0 && name.compare("guid") == 0)
                            state->line.clear();
                        else if(state->depth == 0 && name.compare("pubDate") == 0)
                            state->line.clear();
                        else {
                            std::ostringstream tmp;
                            tmp << "<" << name;
                            while(*atts) {
                                tmp << " " << *(atts++);
                                tmp << "=\"" << *(atts++) << "\"";
                            }
                            tmp << ">";
                            state->line.append(tmp.str());
                        }
                        state->depth++;
                    }
                }
                static void end_element(void *user_data,
                                        const XML_Char *tag_name) {
                    RssParser *state = static_cast<RssParser *>(user_data);
                    
                    std::string name(tag_name);
                    if(name.compare("item") == 0) {
                        state->line.clear();
                        state->in_item = false;
                        if(state->min_date >= 0 && state->items.front()->date_ts() < state->min_date) {
                            state->items.pop_front();
                        }
                    } else if(state->in_item) {
                        state->depth--;
                        if(state->depth == 0 && name.compare("title") == 0)
                            state->items.front()->title(state->line);
                        else if(state->depth == 0 && name.compare("link") == 0)
                            state->items.front()->link(state->line);
                        else if(state->depth == 0 && name.compare("description") == 0)
                            state->items.front()->description(state->line);
                        else if(state->depth == 0 && name.compare("author") == 0)
                            state->items.front()->author(state->line);
                        else if(state->depth == 0 && name.compare("guid") == 0)
                            state->items.front()->guid(state->line);
                        else if(state->depth == 0 && name.compare("pubDate") == 0)
                            state->items.front()->date(state->line);
                        else {
                            std::ostringstream tmp;
                            tmp << "</" << name << ">";
                            state->line.append(tmp.str());
                        }
                    }
                }
                static void character_data(void *user_data,
                                           const XML_Char *data,
                                           int len) {
                    RssParser *state = static_cast<RssParser *>(user_data);
                    state->line.append(data, len);
                }
            };
            
            size_t itembuilder_fetch(void *ptr, size_t size, size_t nmemb, void *stream) {
                XML_Parser p = static_cast<XML_Parser>(stream);
                if(!p) return 0;
                
                size_t bytes = size * nmemb;
                
                void *buffer = XML_GetBuffer(p, bytes);
                if(!buffer) {
                    std::cerr << "Unable to allocate buffer ";
                    std::cerr << XML_ErrorString(XML_GetErrorCode(p)) << std::endl;
                    return 0;
                }
                
                memcpy(buffer, ptr, bytes);
                if(!XML_ParseBuffer(p, bytes, false)) {
                    std::cerr << "Unable to parse buffer ";
                    std::cerr << XML_ErrorString(XML_GetErrorCode(p)) << std::endl;
                    return 0;
                }
                return bytes;
            }
        };
        
        void CommitFeedController::execute(CGI::Request *request, CGI::Response *response) {
            std::list<std::string> args(request->split_path_info());
            
            // Remove the command name.
            args.pop_back();
            
            User *user = request->context_object<User>("_user");
            
            if(request->is_post()) {
                long long last_commit = 0;
                // Loop over the submitted fields.
                CGI::Request::param_map::const_iterator end(request->params().upper_bound("taskAssignments-zzzz"));
                for(CGI::Request::param_map::const_iterator iter = request->params().lower_bound("taskAssignments-");
                    iter != end;
                    ++iter) {
                    // Prepare the other keys we will need.
                    std::string commit_time_key(iter->first);
                    commit_time_key.replace(0, 15, "taskDate");
                    std::string commit_comment_key(iter->first);
                    commit_comment_key.replace(0, 15, "taskComment");
                    
                    // Take care of the commit time.
                    long long commit_time = atol(request->param(commit_time_key).c_str());
                    if(commit_time > last_commit) last_commit = commit_time;
                    
                    Backlog b;
                    Backlog::at(atol(iter->second.c_str()), &b);
                    
                    std::ostringstream comment;
                    comment << user->name() << ": " << request->param(commit_comment_key);
                    b.comments().push_back(comment.str());
                    
                    std::ostringstream assigned_name;
                    assigned_name << "assigned:" << user->name();
                    b.tags().insert(assigned_name.str());
                    
                    std::ostringstream assigned_pkey;
                    assigned_pkey << "assigned:" << user->pkey();
                    b.tags().insert(assigned_pkey.str());
                    
                    if(atol(b.disposition().substr(0,3).c_str()) < 300)
                        b.disposition("300-ASSIGNED");
                    
                    if(b.actual() < 0.1)
                        b.actual(b.estimate());
                    
                    b.save();
                }
                user->last_commit(last_commit);
                user->save();
            }
            
            
            Project p(atol(args.front().c_str()));
            char error_buffer[CURL_ERROR_SIZE];
            XML_Parser parser;
            RssParser parser_state(user->last_commit());
            
            parser = XML_ParserCreate("UTF-8");
            XML_SetUserData(parser, &parser_state);
            XML_SetElementHandler(parser, &RssParser::start_element, &RssParser::end_element);
            XML_SetCharacterDataHandler(parser, &RssParser::character_data);
            
            CURL *curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &itembuilder_fetch);
            curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
            curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 15L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "LogJammin v1.0 (using cURL)");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, parser);
            curl_easy_setopt(curl, CURLOPT_URL, p.commit_feed().c_str());
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
            
            // Peform the CURL operation.
            if(curl_easy_perform(curl)) {
                curl_easy_cleanup(curl);
                throw std::string(p.commit_feed()).append(" Error ").append(error_buffer);
            }
            
            // Test the response code.
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if(response_code != 200) {
                curl_easy_cleanup(curl);
                std::ostringstream msg;
                msg << "Unexpected response code " << response_code << ".";
                throw msg.str();
            }
            
            if(!XML_GetBuffer(parser, 1)) {
                std::ostringstream msg;
                msg << "Unable to finish allocating buffer ";
                msg << XML_ErrorString(XML_GetErrorCode(parser)) << std::endl;
                throw msg.str();
            }
            
            if(!XML_ParseBuffer(parser, 0, true)) {
                std::ostringstream msg;
                msg << "Unable to finish parsing buffer ";
                msg << XML_ErrorString(XML_GetErrorCode(parser)) << std::endl;
                throw msg.str();
            }
            
            // Clean up after curl.
            curl_easy_cleanup(curl);
            
            request->context_object("project", &p, false);
            request->context_object_list("rss_items", parser_state.items, true);
            response->execute("commit-feed.html", request);
            request->attribute("handled", "true");
        }
    };
};
