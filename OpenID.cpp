/*
 \file OpenID.cpp
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

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <list>
#include <map>
extern "C" {
#include <curl/curl.h>
#include <openssl/hmac.h>
}
#include "OpenID.h"

namespace openid_1_1 {
    
    namespace {
        size_t stringbuilder_fetch(void *ptr, size_t size, size_t nmemb, void *stream) {
            std::string *content = static_cast<std::string *>(stream);
            if(!content) return 0;
            
            size_t bytes = size * nmemb;
            content->append((const char *)ptr, bytes);
            return bytes;
        }
        
        CURL *new_curl_handle() {
            CURL *curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &stringbuilder_fetch);
            curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
            curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 15L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "OpenID Auth 1.1 Consumer (using cURL)");
            return curl;
        }
        
        // Simple uninteligent implementation.
        unsigned char *base64_decode(const std::string &input, unsigned int *size) {
            const char values[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\0";
            std::map<char, unsigned char> base64_table;
            char *current_value = const_cast<char *>(values);
            int i = 0;
            while(*current_value) {
                base64_table[*(current_value++)] = i++;
            }
            
            std::list<unsigned char> buffer;
            std::string::const_iterator iter = input.begin();
            while(iter != input.end() && *iter != '=') {
                char tmp[4];
                i = 0;
                for(; iter != input.end() && *iter != '=' && i < 4; ++i, ++iter) {
                    tmp[i] = base64_table.find(*iter)->second;
                }
                
                switch(i) {
                    case 4:
                        buffer.push_back((unsigned char)((tmp[0] << 2) | (tmp[1] >> 4)));
                        buffer.push_back((unsigned char)((tmp[1] << 4) | (tmp[2] >> 2)));
                        buffer.push_back((unsigned char)((tmp[2] << 6) | tmp[3]));
                        break;
                    case 3:
                        buffer.push_back((unsigned char)((tmp[0] << 2) | (tmp[1] >> 4)));
                        buffer.push_back((unsigned char)((tmp[1] << 4) | (tmp[2] >> 2)));
                        break;
                    case 2:
                        buffer.push_back((unsigned char)((tmp[0] << 2) | (tmp[1] >> 4)));
                        break;
                    case 1:
                        throw std::string("Invalid base64 string. Incomplete final character.");
                }
            }
            
            *size = buffer.size();
            unsigned char *result = new unsigned char[*size];
            
            i = 0;
            for(std::list<unsigned char>::const_iterator iter = buffer.begin();
                iter != buffer.end();
                ++iter, ++i) {
                result[i] = *iter;
            }
            
            return result;
        }
        
        std::string base64_encode(const unsigned char *input, unsigned int size) {
            std::ostringstream data;
            const char values[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            int h = 0;
            while(h < size - 2) {
                data << values[input[h] >> 2];
                data << values[((input[h] & 3) << 4)  | (input[h + 1] >> 4)];
                data << values[((input[h + 1] & 15) << 2) | (input[h + 2] >> 6)];
                data << values[input[h + 2] & 63];
                h += 3;
            }
            
            switch(size - h) {
                case 1:
                    data << values[input[h] >> 2];
                    data << values[(input[h] & 3) << 4];
                    data << "==";
                    break;
                case 2:
                    data << values[input[h] >> 2];
                    data << values[((input[h] & 3) << 4)  | (input[h + 1] >> 4)];
                    data << values[(input[h + 1] & 15) << 2];
                    data << "=";
                    break;
            }
            
            return data.str();
        }
        
        unsigned char *create_signature(const std::string &value, unsigned char *secret, unsigned int secret_size, unsigned int *s) {
            const EVP_MD *md = EVP_sha1();
            unsigned char * md_value = new unsigned char[EVP_MAX_MD_SIZE];
            
            HMAC(md,
                 secret,
                 secret_size,
                 (const unsigned char *)(value.c_str()),
                 value.size(),
                 md_value,
                 s);
            
            return md_value;
        }
        
        std::string create_signature(const std::string &value, const std::string secret) {
            unsigned int sz, s;
            unsigned char *secret_bytes = base64_decode(secret, &sz);
            unsigned char *digest = create_signature(value, secret_bytes, sz, &s);
            delete secret_bytes;
            std::string result(base64_encode(digest, s));
            delete digest;
            return result;
        }
    }
    
    DumbRelayConsumer::DumbRelayConsumer(const std::string &identifier) {
        this->identifier(identifier);
        discovery();
    }
    
    void DumbRelayConsumer::identifier(const std::string &identifier) {
        bool needs_prefix = true;
        
        // check for the existance of a prefix
        if(identifier.size() > 11 &&
           (identifier.at(0) == 'H' || identifier.at(0) == 'h') &&
           (identifier.at(1) == 'T' || identifier.at(1) == 't') &&
           (identifier.at(2) == 'T' || identifier.at(2) == 't') &&
           (identifier.at(3) == 'P' || identifier.at(3) == 'p'))
            if(identifier.at(4) == ':' &&
               identifier.at(5) == '/' &&
               identifier.at(6) == '/') {
                needs_prefix = false;
            } else if((identifier.at(4) == 'S' || identifier.at(4) == 's') &&
                      identifier.at(4) == ':' &&
                      identifier.at(5) == '/' &&
                      identifier.at(6) == '/') {
                needs_prefix = false;
            }
        
        // Set the final identifier.
        _identifier.clear();
        if(needs_prefix)
            _identifier.append("http://");
        _identifier.append(identifier);
    }
    
    void DumbRelayConsumer::openid_provider(const std::string &openid_provider) {
        _openid_provider = openid_provider;
    }
    
    void DumbRelayConsumer::discovery() {
        // vars for cURL.
        char error_buffer[CURL_ERROR_SIZE];
        std::string content;
        
        // Configure the cURL handle.
        CURL *curl = new_curl_handle();
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
        curl_easy_setopt(curl, CURLOPT_URL, identifier().c_str());
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        
        // Peform the CURL operation.
        if(curl_easy_perform(curl)) {
            curl_easy_cleanup(curl);
            throw std::string(error_buffer);
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
        
        // Replace the given identifier with where we actually got the page from.
        // Curl gives us a pointer into itself, do not free.
        char *final_url;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &final_url);
        identifier(final_url);
        
        // Clean up after curl, we don't need it for the rest of this method.
        curl_easy_cleanup(curl);
        
        // Going to loop over all the content and try to find link tags that are
        // relevant to us.
        for(std::string::const_iterator iter = content.begin();
            iter != content.end();
            ++iter) {
            
            // Look for "< LINK "
            if(*iter == '<') {
                // Skip any white space.
                do {
                    ++iter;
                } while(iter != content.end() && (*iter == ' ' || *iter == '\n' || *iter == '\r' || *iter == '\t'));
                if(iter == content.end()) break;
                
                if(*iter == 'L' || *iter == 'l')
                    if(++iter != content.end() && (*iter == 'I' || *iter == 'i'))
                        if(++iter != content.end() && (*iter == 'N' || *iter == 'n'))
                            if(++iter != content.end() && (*iter == 'K' || *iter == 'k'))
                                if(++iter != content.end() && *iter == ' ') {
                                    // Eureka! we found a link tag.
                                    std::string href, rel;
                                    while(iter != content.end() && *iter != '>') {
                                        if(*iter == 'R' || *iter == 'r') {
                                            if(++iter != content.end() && (*iter == 'E' || *iter == 'e'))
                                                if(++iter != content.end() && (*iter == 'L' || *iter == 'l')) {
                                                    // Skip to the relation attribute value..
                                                    do {
                                                        ++iter;
                                                    } while(iter != content.end() && (*iter == ' ' || *iter == '\n' || *iter == '\r' || *iter == '\t' || *iter == '='));
                                                    if(iter == content.end()) break;
                                                    
                                                    // Deal with quoted vs. unquoted.
                                                    char end_char = ' ';
                                                    if(*iter == '\"') {
                                                        end_char = '\"';
                                                        ++iter;
                                                    }
                                                    
                                                    // Read the value.
                                                    while(iter != content.end() && *iter != end_char) {
                                                        rel.push_back(*iter);
                                                        ++iter;
                                                    }
                                                }
                                            
                                        } else if(*iter == 'H' || *iter == 'h') {
                                            if(++iter != content.end() && (*iter == 'R' || *iter == 'r'))
                                                if(++iter != content.end() && (*iter == 'E' || *iter == 'e'))
                                                    if(++iter != content.end() && (*iter == 'F' || *iter == 'f')) {
                                                        // Skip to the relation attribute value..
                                                        do {
                                                            ++iter;
                                                        } while(iter != content.end() && (*iter == ' ' || *iter == '\n' || *iter == '\r' || *iter == '\t' || *iter == '='));
                                                        if(iter == content.end()) break;
                                                        
                                                        // Deal with quoted vs. unquoted.
                                                        char end_char = ' ';
                                                        if(*iter == '\"') {
                                                            end_char = '\"';
                                                            ++iter;
                                                        }
                                                        
                                                        // Read the value.
                                                        while(iter != content.end() && *iter != end_char) {
                                                            href.push_back(*iter);
                                                            ++iter;
                                                        }
                                                    }
                                        } else {
                                            ++iter;
                                        }
                                    }
                                    if((rel.size() == 13 || rel.size() == 15) &&
                                       (rel.at(0) == 'O' || rel.at(0) == 'o') &&
                                       (rel.at(1) == 'P' || rel.at(1) == 'p') &&
                                       (rel.at(2) == 'E' || rel.at(2) == 'e') &&
                                       (rel.at(3) == 'N' || rel.at(3) == 'n') &&
                                       (rel.at(4) == 'I' || rel.at(4) == 'i') &&
                                       (rel.at(5) == 'D' || rel.at(5) == 'd') &&
                                       (rel.at(6) == '.')) {
                                        if(rel.size() == 13 &&
                                           (rel.at(7) == 'S' || rel.at(7) == 's') &&
                                           (rel.at(8) == 'E' || rel.at(8) == 'e') &&
                                           (rel.at(9) == 'R' || rel.at(9) == 'r') &&
                                           (rel.at(10) == 'V' || rel.at(10) == 'v') &&
                                           (rel.at(11) == 'E' || rel.at(11) == 'e') &&
                                           (rel.at(12) == 'R' || rel.at(12) == 'r')) {
                                            openid_provider(href);
                                        } else if(rel.size() == 15 &&
                                                  (rel.at(7) == 'D' || rel.at(7) == 'd') &&
                                                  (rel.at(8) == 'E' || rel.at(8) == 'e') &&
                                                  (rel.at(9) == 'L' || rel.at(9) == 'l') &&
                                                  (rel.at(10) == 'E' || rel.at(10) == 'e') &&
                                                  (rel.at(11) == 'G' || rel.at(11) == 'g') &&
                                                  (rel.at(12) == 'A' || rel.at(12) == 'a') &&
                                                  (rel.at(13) == 'T' || rel.at(13) == 't') &&
                                                  (rel.at(14) == 'E' || rel.at(14) == 'e')) {
                                            identifier(href);
                                        }
                                    }
                                    if(iter == content.end()) break;
                                }
                
            }
        }
    }
    
    std::string DumbRelayConsumer::contact_openid_provider(const std::string &post_data) {
        CURL *curl = new_curl_handle();
        // Prepare the curl handle.
        char error_buffer[CURL_ERROR_SIZE];
        std::string content;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
        curl_easy_setopt(curl, CURLOPT_URL, openid_provider().c_str());
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, post_data.c_str());
        
        // Peform the CURL operation.
        if(curl_easy_perform(curl)) {
            curl_easy_cleanup(curl);
            throw std::string(error_buffer);
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
        
        // Clean up after curl.
        curl_easy_cleanup(curl);
        
        // Check to see if we can find the is_valid:true response.
        return content;
    }
    
    std::string DumbRelayConsumer::checkid_setup(const std::string &return_to,
                                                 const std::string &trust_root) {
        // construct check_id url.
        char *tmp;
        std::string redirect_url(openid_provider());
        if(redirect_url.find_first_of('?') != redirect_url.npos)
            redirect_url.push_back('&');
        else
            redirect_url.push_back('?');

        redirect_url.append("openid.mode=checkid_setup");
        
        // cURL handle used for escaping.
        CURL *curl = new_curl_handle();
        redirect_url.append("&openid.identity=");
        tmp = curl_easy_escape(curl, identifier().c_str(), identifier().size());
        redirect_url.append(tmp);
        curl_free(tmp);
        
        redirect_url.append("&openid.return_to=");
        tmp = curl_easy_escape(curl, return_to.c_str(), return_to.size());
        redirect_url.append(tmp);
        curl_free(tmp);
        
        if(trust_root.size() > 0) {
            redirect_url.append("&openid.trust_root=");
            tmp = curl_easy_escape(curl, trust_root.c_str(), trust_root.size());
            redirect_url.append(tmp);
            curl_free(tmp);
        }
        
        // Clean up after curl.
        curl_easy_cleanup(curl);        
        
        return redirect_url;
    }
    
    bool DumbRelayConsumer::check_authentication(const std::multimap<std::string, std::string> &params) {
        // Construct the data for the post.
        std::ostringstream data;
        CURL *curl = new_curl_handle();
        data << "openid.mode=check_authentication";
        for(std::multimap<std::string, std::string>::const_iterator iter = params.begin();
            iter != params.end();
            ++iter) {
            if(iter->first.compare("openid.mode") == 0) continue;
            
            char *tmp;
            tmp = curl_easy_escape(curl, iter->first.c_str(), iter->first.size());
            data << "&" << tmp << "=";
            curl_free(tmp);
            
            tmp = curl_easy_escape(curl, iter->second.c_str(), iter->second.size());
            data << tmp;
            curl_free(tmp);
        }
        // Clean up after curl.
        curl_easy_cleanup(curl);
        
        // Prepare the curl handle.
        std::string content = contact_openid_provider(data.str());
        
        // Check to see if we can find the is_valid:true response.
        return content.find("\nis_valid:true\n") != content.npos;
    }
    
    AssociatedRelayConsumer::AssociatedRelayConsumer(const std::string &identifier) : DumbRelayConsumer(identifier) {
    }
    
    std::string AssociatedRelayConsumer::checkid_setup(const std::string &return_to,
                                                       const std::string &trust_root) {
        // Try to find our assoc handle.
        const std::string *assoc_handle_ptr = lookup_assoc_handle(openid_provider());
        if(!assoc_handle_ptr)
            assoc_handle_ptr = associate();
        
        // If the handle is "DUMB", we have to fall back to the dumb consumer.
        if(assoc_handle_ptr->compare("DUMB") == 0) {
            delete assoc_handle_ptr;
            return DumbRelayConsumer::checkid_setup(return_to, trust_root);
        }
        
        // Move the string to the stack, so that return can free it.
        std::string assoc_handle(*assoc_handle_ptr);
        delete assoc_handle_ptr;
        
        // construct check_id url.
        std::string redirect_url(DumbRelayConsumer::checkid_setup(return_to, trust_root));

        // cURL handle used for escaping.
        CURL *curl = new_curl_handle();
        redirect_url.append("&openid.assoc_handle=");
        char *tmp = curl_easy_escape(curl, assoc_handle.c_str(), assoc_handle.size());
        redirect_url.append(tmp);
        curl_free(tmp);
        
        // Clean up after curl.
        curl_easy_cleanup(curl);
        
        return redirect_url;
    }
    
    bool AssociatedRelayConsumer::check_authentication(const std::multimap<std::string, std::string> &params) {
        // Try to find our assoc handle.
        const std::string *assoc_handle_ptr = lookup_assoc_handle(openid_provider());
        if(!assoc_handle_ptr)
            assoc_handle_ptr = associate();
        
        // If the handle is "DUMB", we have to fall back to the dumb consumer.
        if(assoc_handle_ptr->compare("DUMB") == 0) {
            delete assoc_handle_ptr;
            return DumbRelayConsumer::check_authentication(params);
        }
        
        // Move the string to the stack, so that return can free it.
        std::string assoc_handle(*assoc_handle_ptr);
        delete assoc_handle_ptr;
        
        // Not possible to be missing the assoc_handle
        std::multimap<std::string, std::string>::const_iterator assoc_iter = params.find(std::string("openid.assoc_handle"));
        if(assoc_iter == params.end())
            return false;
        
        if(assoc_handle.compare(assoc_iter->second) == 0) {
            // make sure this request was signed.
            std::multimap<std::string, std::string>::const_iterator sig_iter = params.find(std::string("openid.sig"));
            if(sig_iter == params.end())
                return false;
            std::string their_signature(sig_iter->second);
            
            // Look for the signed fields.
            sig_iter = params.find(std::string("openid.signed"));
            if(sig_iter == params.end())
                return false;
            std::string signed_params(sig_iter->second), key;
            
            // construct the message that was signed.
            std::ostringstream message;
            std::string::const_iterator iter = signed_params.begin();
            while(iter != signed_params.end()) {
                char c = *iter;
                ++iter;
                
                // We hit a non-key character, 
                if(c == '\n' || c == ',' || iter == signed_params.end()) {
                    // store the last char in the string.
                    if(iter == signed_params.end()) key.push_back(c);
                    // look for the param
                    std::string key_to_find("openid.");
                    key_to_find.append(key);
                    sig_iter = params.find(key_to_find);
                    if(sig_iter == params.end())
                        return false;
                    
                    // add to the message data.
                    message << key << ":" << sig_iter->second << "\n";
                    
                    // clear the key to start over.
                    key.clear();
                } else {
                    key.push_back(c);
                }
            }
            
            // attempt to recreate the signature.
            Association *assoc = lookup_association(assoc_handle);
            if(!assoc)
                return false;
            std::string our_signature(create_signature(message.str(), assoc->secret));
            delete assoc;

            // Test that the signature created matches the one given.
            return (their_signature.compare(our_signature) == 0);
            
        } else if(assoc_handle.compare(assoc_iter->second) != 0 &&
           (assoc_iter = params.find("openid.invalidate_handle")) != params.end()) {
            
            // Prepare the verification post data.
            std::ostringstream data;
            CURL *curl = new_curl_handle();
            data << "openid.mode=check_authentication";
            for(std::multimap<std::string, std::string>::const_iterator iter = params.begin();
                iter != params.end();
                ++iter) {
                if(iter->first.compare("openid.mode") == 0) continue;
                
                char *tmp;
                tmp = curl_easy_escape(curl, iter->first.c_str(), iter->first.size());
                data << "&" << tmp << "=";
                curl_free(tmp);
                
                tmp = curl_easy_escape(curl, iter->second.c_str(), iter->second.size());
                data << tmp;
                curl_free(tmp);
            }
            // Clean up after curl.
            curl_easy_cleanup(curl);
            
            // execute.
            std::string content = contact_openid_provider(data.str());
            
            // Check for invalidating the handle.
            std::string invalidate_substring("\ninvalidate_handle:");
            invalidate_substring.append(assoc_iter->second).append("\n");
            if(content.find(invalidate_substring) != content.npos)
                invalidate_assoc_handle(assoc_iter->second);
            
            // Test for is_valid.
            return content.find("\nis_valid:true\n") != content.npos;
        }
        
        // If we didn't match any other cases, return false.
        return false;
    }
    
    std::string *AssociatedRelayConsumer::associate() {
        // Construct the data for the post.
        std::ostringstream data;
        data << "openid.mode=associate";
        data << "&openid.assoc_type=HMAC-SHA1";
        data << "&openid.session_type=";
        
        // Contact the server.
        std::string content = contact_openid_provider(data.str());
        
        // Parse the results.
        std::string key_str, line;
        bool key = true;
        Association assoc;
        for(std::string::const_iterator iter = content.begin();
            iter != content.end();
            ++iter) {
            if(!key && *iter == '\n') {
                if(key_str.compare("assoc_type") == 0)
                    assoc.assoc_type = line;
                else if(key_str.compare("assoc_handle") == 0)
                    assoc.assoc_handle = line;
                else if(key_str.compare("expires_in") == 0)
                    assoc.expires_at = atol(line.c_str()) + time(NULL);
                else if(key_str.compare("session_type") == 0)
                    assoc.session_type = line;
                else if(key_str.compare("dh_server_public") == 0)
                    assoc.dh_server_public = line;
                else if(key_str.compare("mac_key") == 0)
                    assoc.secret = line;
                else if(key_str.compare("enc_mac_key") == 0)
                    assoc.secret = line;
                line.clear();
                key = true;
            } else if(key && *iter == ':') {
                key_str = line;
                key = false;
                line.clear();
            } else {
                line.push_back(*iter);
            }
        }
        assoc.provider = openid_provider();
        
        // Store the assoc
        store_assoc_handle(&assoc);
        
        return new std::string(assoc.assoc_handle);
    }
};
