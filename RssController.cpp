extern "C" {
#include <curl/curl.h>
#include <expat.h>
}
#include <list>
#include "Project.h"
#include "RssItem.h"
#include "RssController.h"

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
    public:
        std::list<RssItem *> items;
        RssParser() : in_item(false) { };
        static void start_element(void *user_data,
                                  const XML_Char *tag_name,
                                  const XML_Char **atts) {
            RssParser *state = static_cast<RssParser *>(user_data);
            
            std::string name(tag_name);
            if(name.compare("item") == 0) {
                state->items.push_back(new RssItem());
                state->in_item = true;
            } else if(state->in_item) {
                if(name.compare("title") == 0)
                    state->line.clear();
                else if(name.compare("link") == 0)
                    state->line.clear();
                else if(name.compare("description") == 0)
                    state->line.clear();
                else if(name.compare("author") == 0)
                    state->line.clear();
                else if(name.compare("guid") == 0)
                    state->line.clear();
                else if(name.compare("pubDate") == 0)
                    state->line.clear();
            }
        }
        static void end_element(void *user_data,
                                const XML_Char *tag_name) {
            RssParser *state = static_cast<RssParser *>(user_data);
            
            std::string name(tag_name);
            if(name.compare("item") == 0) {
                state->line.clear();
                state->in_item = false;
            } else if(state->in_item) {
                if(name.compare("title") == 0) {
                    state->items.back()->title(state->line);
                } else if(name.compare("link") == 0) {
                    state->items.back()->link(state->line);
                } else if(name.compare("description") == 0) {
                    state->items.back()->description(state->line);
                } else if(name.compare("author") == 0) {
                    state->items.back()->author(state->line);
                } else if(name.compare("guid") == 0) {
                    state->items.back()->guid(state->line);
                } else if(name.compare("pubDate") == 0) {
                    state->items.back()->date(state->line);
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
    
    size_t stringbuilder_fetch(void *ptr, size_t size, size_t nmemb, void *stream) {
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
    
    Project p(atol(args.front().c_str()));
    char error_buffer[CURL_ERROR_SIZE];
    XML_Parser parser;
    RssParser parser_state;
    
    parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &parser_state);
    XML_SetElementHandler(parser, &RssParser::start_element, &RssParser::end_element);
    XML_SetCharacterDataHandler(parser, &RssParser::character_data);
    
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &stringbuilder_fetch);
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




