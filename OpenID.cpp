#include <iostream>
#include <sstream>
#include <cstdlib>
#include "curl/curl.h"
#include "OpenID.h"
extern "C" {
#include <openssl/hmac.h>
}

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
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "LogJammin 1.0 (OpenID Auth 1.1 Consumer)");
            return curl;
        }
        
        unsigned char * base64_decode(const std::string &input, unsigned int *size) {
            int i = 0;
            unsigned char *result = new unsigned char[(input.size() / 2)], c;
            
            std::string::const_iterator iter = input.begin();
            while(iter != input.end()) {
                const char hex[3] = {*++iter, *++iter, '\0'};
                c = (unsigned char)strtol(hex, NULL, 16);
                result[i++] = c;
            }
            
            *size = i;
            return result;
        }
        
        std::string base64_encode(const unsigned char *input, unsigned int size) {
            std::ostringstream data;
            for(int h = 0; h < size; ++h) {
                char hex[3];
                sprintf(hex, "%02x", input[h]);
                data << hex;
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
    
    DumbRelayProvider::DumbRelayProvider(const std::string &identifier) {
        this->identifier(identifier);
        discovery();
    }
    
    void DumbRelayProvider::identifier(const std::string &identifier) {
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
    
    void DumbRelayProvider::openid_provider(const std::string &openid_provider) {
        _openid_provider = openid_provider;
    }
    
    void DumbRelayProvider::discovery() {
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
    
    std::string DumbRelayProvider::contact_openid_provider(const std::string &post_data) {
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
    
    std::string DumbRelayProvider::checkid_setup(const std::string &return_to,
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
    
    bool DumbRelayProvider::check_authentication(const std::multimap<std::string, std::string> &params) {
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
    
    AssociatedRelayProvider::AssociatedRelayProvider(const std::string &identifier) : DumbRelayProvider(identifier) {
        std::cerr << "Creating a new Associated Relay Provider " << this->identifier() << std::endl;
    }
    
    std::string AssociatedRelayProvider::checkid_setup(const std::string &return_to,
                                                       const std::string &trust_root) {
        
        std::cerr << "Check ID Setup return to " << return_to << " trust root " << trust_root << std::endl;

        std::string *assoc_handle = lookup_assoc_handle(openid_provider());
        if(!assoc_handle)
            assoc_handle = associate();
        if(assoc_handle->compare("DUMB") == 0)
            assoc_handle = NULL;

        std::cerr << "Assoc Handle " << (assoc_handle ? *assoc_handle : "NULL") << "." << std::endl;

        // construct check_id url.
        std::string redirect_url(DumbRelayProvider::checkid_setup(return_to, trust_root));

        std::cerr << "Redirect URL from Dumb " << redirect_url << std::endl;

        // cURL handle used for escaping.
        CURL *curl = new_curl_handle();
        if(assoc_handle) {
            redirect_url.append("&openid.assoc_handle=");
            char *tmp = curl_easy_escape(curl, assoc_handle->c_str(), assoc_handle->size());
            redirect_url.append(tmp);
            curl_free(tmp);
        }
        
        // Clean up after curl.
        curl_easy_cleanup(curl);        
        
        std::cerr << "final Redirect " << redirect_url << std::endl;
        
        return redirect_url;
    }
    
    bool AssociatedRelayProvider::check_authentication(const std::multimap<std::string, std::string> &params) {
        std::string *assoc_handle = lookup_assoc_handle(openid_provider());
        if(!assoc_handle)
            assoc_handle = associate();
        if(assoc_handle->compare("DUMB") == 0)
            assoc_handle = NULL;

        std::cerr << "Assoc Handle " << (assoc_handle ? *assoc_handle : "NULL") << "." << std::endl;
        
        if(!assoc_handle) {
            std::cerr << "Invoking Dumb Check " << std::endl;
            return DumbRelayProvider::check_authentication(params);
        }
        
        // Not possible to be missing the assoc_handle
        std::multimap<std::string, std::string>::const_iterator assoc_iter = params.find(std::string("openid.assoc_handle"));
        if(assoc_iter == params.end())
            return false;
        
        std::cerr << "Assoc iter " << assoc_iter->second << std::endl;
        std::cerr << "assoc_handle compare to Assoc iter " << assoc_handle->compare(assoc_iter->second) << std::endl;
        
        if(assoc_handle->compare(assoc_iter->second) == 0) {
            // make sure this request was signed.
            std::multimap<std::string, std::string>::const_iterator sig_iter = params.find(std::string("openid.sig"));
            if(sig_iter == params.end())
                return false;
            std::string their_signature(sig_iter->second);
            
            std::cerr << "Their Signature " << their_signature << std::endl;
            
            // Look for the signed fields.
            sig_iter = params.find(std::string("openid.signed"));
            if(sig_iter == params.end())
                return false;
            std::string signed_params(sig_iter->second), key;

            std::cerr << "Signed Params " << signed_params << std::endl;            
            
            // construct the message that was signed.
            std::ostringstream message;
            std::string::const_iterator iter = signed_params.begin();
            while(iter != signed_params.end()) {
                char c = *iter;
                ++iter;
                if(c == '\n' || c == ',' || iter == signed_params.end()) {
                    // look for the param
                    sig_iter = params.find(std::string("openid.").append(key));
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
            std::cerr << "Params to be signed " << message.str() << std::endl;
            
            // attempt to recreate the signature.
            Association *assoc = lookup_association(*assoc_handle);
            std::string our_signature(create_signature(message.str(), assoc->secret));

            std::cerr << "Our Signature " << our_signature << " bool " << (their_signature.compare(our_signature) == 0) << std::endl;
            
            // Test that the signature created matches the one given.
            return (their_signature.compare(our_signature) == 0);
            
        } else if(assoc_handle->compare(assoc_iter->second) != 0 &&
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
    
    std::string *AssociatedRelayProvider::associate() {
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
        Association *assoc = new Association();
        for(std::string::const_iterator iter = content.begin();
            iter != content.end();
            ++iter) {
            if(!key && *iter == '\n') {
                if(key_str.compare("assoc_type") == 0)
                    assoc->assoc_type = line;
                else if(key_str.compare("assoc_handle") == 0)
                    assoc->assoc_handle = line;
                else if(key_str.compare("expies_in") == 0)
                    assoc->expires_at = 0;
                else if(key_str.compare("session_type") == 0)
                    assoc->session_type = line;
                else if(key_str.compare("dh_server_public") == 0)
                    assoc->dh_server_public = line;
                else if(key_str.compare("mac_key") == 0)
                    assoc->secret = line;
                else if(key_str.compare("enc_mac_key") == 0)
                    assoc->secret = line;
                key = true;
            } else if(key && *iter == ':') {
                key_str = line;
                key = false;
                line.clear();
            } else {
                line.push_back(*iter);
            }
        }
        assoc->provider = openid_provider();
        
        // Store the assoc
        store_assoc_handle(assoc);
        
        return new std::string("DUMB");
    }
};
