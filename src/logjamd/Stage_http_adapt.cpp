/*!
 \file Stage_http_adapt.cpp
 \brief Logjam server stage for converting telnet json input into bson input.
 \author Jason Watson

 Copyright (c) 2010, Jason Watson
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

#include "logjamd/Stage_http_adapt.h"
#include "logjamd/Stage_auth.h"
#include "logjamd/constants.h"
#include "lj/Base64.h"
#include "lj/Log.h"
#include "lj/Streambuf_pipe.h"

#include <cassert>
#include <map>
#include <iostream>

namespace
{
    const std::string HTTP_VERSION_PREFIX("HTTP/");
    const std::string HTTP_VERSION_1_0("HTTP/1.0");
    const std::string HTTP_VERSION_1_1("HTTP/1.1");
    const std::string REQUIRE_AUTH_PREFIX("~/");
    const std::string HEADER_LINE_ENDING("\r\n");
    const std::string HEADER_CONTENT_LENGTH("Content-Length: ");
    const std::string HEADERS_AUTH_REQUIRED("HTTP/1.0 401 Unauthorized\r\nServer: Logjamd\r\nContent-Type: application/json; charset=\"UTF-8\"\r\nWWW-Authenticate: Basic realm=\"Secure Command Execution\"\r\n");
    const std::string HEADERS_FORBIDDEN("HTTP/1.0 403 Forbidden\r\nServer: Logjamd\r\nContent-Type: application/json; charset=\"UTF-8\"\r\n");
    const std::string HEADERS_SERVER_ERROR("HTTP/1.0 500 Internal Server Error\r\nServer: Logjamd\r\nContent-Type: application/json; charset=\"UTF-8\"\r\n");
    const std::string HEADERS_SUCCESS("HTTP/1.0 200 OK\r\nContent-Type: application/json; charset=\"UTF-8\"\r\n");

    
    std::string percent_decode(const std::string& input)
    {
        std::string result;
        for(auto iter = input.begin();
                input.end() != iter;
                ++iter)
        {
            char c = *iter;
            
            // unescape some control characters.
            if (c == '+')
            {
                c = ' ';
            }
            else if (c == '%')
            {
                const char hex[3] = {*++iter, *++iter, '\0'};
                c = (char)strtol(hex, NULL, 16);
                if(!c) c = '?';
            }
            
            // Test the unescaped character.
            if(c == '\r')
            {
                continue;
            }
            result.push_back(c);
        }
        return result;
    }
    
    void header_to_key_value(const std::string& header,
            std::string& key,
            std::string& value)
    {
        size_t location = header.find_first_of(":");
        if (location == std::string::npos)
        {
            // if there is no colon, assume this whole thing is a key.
            key.assign(header);
            value.erase();
        }
        else
        {
            // Split on the first colon.
            key = header.substr(0, location);
            value = header.substr(location + 1);

            // Trim the trailing and leading whitespace from the value.
            size_t value_start;
            for (value_start = 0; value_start < value.size(); ++value_start)
            {
                if (value[value_start] != ' ' && value[value_start] != '\t')
                {
                    break;
                }
            }
            size_t value_end;
            for (value_end = value.size(); value_end > value_start; --value_end)
            {
                if (value[value_start] != ' ' && value[value_start] != '\t')
                {
                    break;
                }
            }
            value = value.substr(value_start, value_end - value_start);
        }
    }
    
    class Http_request : public logjam::Context::Additional_data
    {
    public:
        enum class Method {
            k_get,
            k_put,
            k_post
        };
        Http_request(const std::string& m) :
                header_read_ahead(),
                headers(),
                method_(Method::k_get),
                uri_(),
                http_version_major_(1),
                http_version_minor_(0),
                body_(nullptr)
        {
            if (m.compare("post") == 0)
            {
                method_ = Method::k_post;
            }
            else if (m.compare("put") == 0)
            {
                method_ = Method::k_put;
            }
        }
        ~Http_request()
        {
            if (body_)
            {
                delete[] body_;
            }
        }
        const Method method() const
        {
            return method_;
        }
        const std::string& uri() const
        {
            return uri_;
        }
        void uri(const std::string& val)
        {
            uri_ = val;
        }
        bool compatible_version(int major, int minor)
        {
            // Major version basically defines the message format. Minor version
            // is the level of extension supported.
            // See RFC 2612, 3.1.
            if (version_major() == major)
            {
                if (version_minor() < minor)
                {
                    return true;
                }
            }
            return false;
        }
        int version_major() const
        {
            return http_version_major_;
        }
        void version_major(int major)
        {
            http_version_major_ = major;
        }
        int version_minor() const
        {
            return http_version_minor_;
        }
        void version_minor(int minor)
        {
            http_version_minor_ = minor;
        }
        int64_t content_length() const
        {
            int64_t length = 0;
            auto iter = headers.find("Content-Length");
            if (headers.end() != iter)
            {
                length = atol(iter->second.c_str());
            }
            return length;
        }
        const uint8_t* body() const
        {
            return body_;
        }
        void body(uint8_t* val)
        {
            body_ = val;
        }
        std::string header_read_ahead;
        std::multimap<std::string, std::string> headers;
        std::unique_ptr<logjam::Stage> real_stage;
    private:
        Method method_;
        std::string uri_;
        int http_version_major_;
        int http_version_minor_;
        uint8_t* body_;
    };
    
    //! Method for getting lines from the http connection. Follows the folding and continuation rules of RFC2612.
    size_t get_http_line(Http_request& state, std::iostream& input_stream, std::string& line)
    {
        // We start with any bytes that we read on a previous call.
        line = state.header_read_ahead;
        state.header_read_ahead.erase();
        
        bool loop = true;
        while (loop)
        {
            // Try to read a line from the input stream.
            std::string buffer;
            std::getline(input_stream, buffer, '\n');

            // Handle issues with the connection.
            if (!input_stream.good())
            {
                throw lj::Exception("Http Server",
                        "Read error while getting header.");
            }

            // getline will remove the newline byte at the end of line.
            // this is to remove the '\r' that is left behind.
            size_t size = buffer.size();
            if (size > 0)
            {
                if (buffer[size - 1] == '\r')
                {
                    buffer.erase(size - 1);
                }
            }

            // If the line contained some data, it might be continued.
            // There are several ways to mark a line continuation.
            size = buffer.size();
            if (size > 0)
            {
                // First we see if this line ended with a backslash.
                // In reality, this should only happen if we are in quotes, but
                // we aren't parsing the value to check for that.
                // NOTE, RFC2612, 2.2 & 3.6 state the backslash character should
                // only ever appear when quoted.
                if (buffer[size - 1] == '\\')
                {
                    buffer.erase(size - 1);
                }
                else
                {
                    // Second, we check if the next line starts with Linear White Space (LWS).
                    // See RFC2612, 2.2 for details about LWS and continuations, and
                    // folding.
                    int next_byte = input_stream.get();
                    if (next_byte == '\t' || next_byte == ' ')
                    {
                        // Folding LWS can all be replaced by a single SP according
                        // to RFC2612, 2.2.
                        while (next_byte == '\t' || next_byte == ' ')
                        {
                            next_byte = input_stream.get();
                        }
                        
                        if (next_byte != '\r')
                        {
                            // Collapse the LWS, and loop.
                            buffer.push_back(' ');
                            buffer.push_back(static_cast<char>(next_byte));
                        }
                        else
                        {
                            // If, for whatever reason, our first no LWS is a CR,
                            // we ignore this line and terminate.
                            loop = false;
                        }
                    }
                    else
                    {
                        // This line didn't end with a backslash, and the next
                        // doesn't start with white space.
                        // NOTE: For performance reasons, this should actually be
                        // the first if statement (most likely scenario). I
                        // am trying to verify my logic right now, so it stays
                        // where I wrote it. :)
                        if (next_byte != '\r')
                        {
                            state.header_read_ahead.push_back(static_cast<char>(next_byte));
                        }
                        loop = false;
                    }
                }
            }
            else
            {
                // We didn't read any data for this line, so we don't need to read the next line.
                loop = false;
            }

            // Add the buffer to the header line.
            line.append(buffer);
        }
        return line.size();
    }
    
    void process_first_line(Http_request& state, std::iostream& input_stream)
    {
        std::string cmd;
        get_http_line(state, input_stream, cmd);
        
        size_t indx = cmd.rfind(HTTP_VERSION_PREFIX);
        if (indx != std::string::npos)
        {
            size_t cmd_length = indx - 1;
            indx += HTTP_VERSION_PREFIX.size();
            
            state.uri(cmd.substr(0, cmd_length));
            const std::string version = cmd.substr(indx);
            indx = version.rfind('.');
            if (indx != std::string::npos)
            {
                const std::string major = version.substr(0, indx);
                const std::string minor = version.substr(indx + 1);
                state.version_major(atoi(major.c_str()));
                state.version_minor(atoi(minor.c_str()));
            }
            lj::log::format<lj::Debug>("uri=%s version=%d.%d").end(state.uri(),
                    state.version_major(),
                    state.version_minor());
        }
    }
    
    void process_header_lines(Http_request& state, std::iostream& input_stream)
    {
        std::string buffer;
        while (get_http_line(state, input_stream, buffer) > 0)
        {
            std::string key;
            std::string value;
            header_to_key_value(buffer, key, value);
            state.headers.insert(std::pair<std::string, std::string>(key, value));
            lj::log::format<lj::Debug>("Received Header [%s]: [%s]").end(key, value);
        }
        lj::log::out<lj::Debug>("Done processing headers");
    }
    
    void process_body_lines(Http_request& state, std::iostream& input_stream)
    {
        int64_t content_length = state.content_length();
        if (content_length > 0)
        {
            // Request says there should be some content in the body.
            // Read that content out and store it.
            uint8_t* buffer = new uint8_t[content_length];
            int64_t indx = 0;
            while (indx < content_length)
            {
                // Read the body bytes.
                input_stream.read(reinterpret_cast<char*>(buffer + indx),
                        content_length - indx);
                
                // Handle issues with the connection.
                if (!input_stream.good())
                {
                    throw lj::Exception("Http Server",
                            "Read error while getting body.");
                }
                
                // Move the indx forward.
                indx += input_stream.gcount();
            }
            state.body(buffer);
        }
    }
    
    void process_params(std::multimap<std::string, std::string>& params, const std::string& input)
    {
        std::string key, value;
        bool both = false;
        for(auto iter = input.begin();
                input.end() != iter;
                ++iter)
        {
            if(*iter == '&')
            {
                params.insert(std::multimap<std::string, std::string>::value_type(percent_decode(key), percent_decode(value)));
                key.erase();
                value.erase();
                both = false;
            }
            else if(*iter == '=')
            {
                both = true;
            }
            else
            {
                if(both)
                    value.push_back(*iter);
                else
                    key.push_back(*iter);
            }
        }
        params.insert(std::multimap<std::string, std::string>::value_type(percent_decode(key), percent_decode(value)));
    }
}; // namespace (anonymous)

namespace logjamd
{
    std::unique_ptr<logjam::Stage> Stage_http_adapt::logic(
            logjam::pool::Swimmer& swmr) const
    {
        logjam::Context& ctx = swmr.context();
        std::iostream& http_ios = swmr.io();

        try
        {
            // Read everything out of the http request.
            std::string method_str(
                    lj::bson::as_string(
                            swmr.context().node().nav("http_adapt/method")));
            std::multimap<std::string, std::string> headers;
            Http_request* req = new Http_request(method_str);
            ctx.data(req);
            process_first_line(*req, http_ios);
            process_header_lines(*req, http_ios);
            process_body_lines(*req, http_ios);
            req->real_stage.reset(new Stage_auth());
        }
        catch (const lj::Exception& ex)
        {
            log("unexpected case: [%s]").end(ex);
            std::string body(ex.str());
            http_ios << HEADERS_SERVER_ERROR
                    << HEADER_CONTENT_LENGTH << body.size()
                    << HEADER_LINE_ENDING << HEADER_LINE_ENDING
                    << body;
            http_ios.flush();
            return std::unique_ptr<Stage>(nullptr);
        }

        assert(ctx.data() != nullptr);
        Http_request& req = *(dynamic_cast<Http_request*>(ctx.data()));
        bool can_retry = false;
        lj::bson::Node auth_request;
        try
        {
            if (0 == req.uri().compare(0, REQUIRE_AUTH_PREFIX.size(),
                        REQUIRE_AUTH_PREFIX))
            {
                // Authentication is required. 
                log("Login required for %s.").end(req.uri());

                // Look for authentication headers
                auto auth_header = req.headers.find("Authorization");

                // Deal with missing authentication information
                if (auth_header == req.headers.end())
                {
                    std::string body("Authentication information required.");
                    http_ios << HEADERS_AUTH_REQUIRED
                            << HEADER_CONTENT_LENGTH << body.size()
                            << HEADER_LINE_ENDING << HEADER_LINE_ENDING
                            << body;
                    http_ios.flush();
                    return std::unique_ptr<Stage>(nullptr);
                }

                // setup the auth_request object.
                // TODO This is brittle, and needs to be fixed up.
                std::string encoded_user_data(auth_header->second.substr(6));
                size_t sz;
                uint8_t* data = lj::base64_decode(encoded_user_data, &sz);
                std::string login_data(reinterpret_cast<char*>(data), sz);
                sz = login_data.find_first_of(':');
                auth_request.set_child("method",
                        lj::bson::new_string(k_auth_method_password));
                auth_request.set_child("provider",
                        lj::bson::new_string(k_auth_provider_local));
                auth_request.set_child("data/login",
                        lj::bson::new_string(login_data.substr(0, sz)));
                auth_request.set_child("data/password",
                        lj::bson::new_string(login_data.substr(sz + 1)));

                can_retry = true;
                
                // remove the auth request part of the command
                req.uri(req.uri().substr(2));
            }
            else
            {
                // If the conection is insecure, use default login.
                log("Using insecure adapter authentication.").end();

                auth_request.set_child("method",
                        lj::bson::new_string(k_auth_method_password));
                auth_request.set_child("provider",
                        lj::bson::new_string(k_auth_provider_local));
                auth_request.set_child("data/login",
                        lj::bson::new_string(k_user_login_json));
                auth_request.set_child("data/password",
                        lj::bson::new_string(k_user_password_json));
            }

            // Create the xlator swimmer
            lj::Streambuf_pipe pipe;
            std::iostream xlator_ios(&pipe);
            logjam::pool::utility::Swimmer_xlator swmr_xlator(swmr, xlator_ios);

            // Log into the system.
            log("Authenticating user [%s].").end(
                    lj::bson::as_string(auth_request["data/login"]));
            pipe.sink() << auth_request;
            req.real_stage = safe_execute_stage(req.real_stage, swmr_xlator);
            if (nullptr == req.real_stage)
            {
                throw LJ__Exception("Translated stage abruptly terminated.");
            }

            // extract the system response.
            lj::bson::Node auth_response;
            pipe.source() >> auth_response;

            // Handle login failures.
            if (lj::bson::as_boolean(auth_response["success"]) == false
                    || ctx.user() == logjam::User::k_unknown
                    || req.real_stage == nullptr)
            {
                log("Login unsuccessful. response=%s, user=%s, test=%s, stage=%p")
                        << lj::bson::as_string(auth_response)
                        << ctx.user().id().str()
                        << swmr_xlator.context().user().id().str()
                        << req.real_stage.get()
                        << lj::log::end;
                std::string body(lj::bson::as_json_string(auth_response));
                if (can_retry)
                {
                    // If they provided credentials, they can try again.
                    http_ios << HEADERS_AUTH_REQUIRED;
                }
                else
                {
                    // don't bother retrying,
                    http_ios << HEADERS_FORBIDDEN;
                }
                http_ios << HEADER_CONTENT_LENGTH << body.size()
                        << HEADER_LINE_ENDING << HEADER_LINE_ENDING
                        << body;
                http_ios.flush();
                return std::unique_ptr<Stage>(nullptr);
            }

            // we got here on a successful login.
            log("Login successful.").end();

            lj::bson::Node request;
            if (Http_request::Method::k_get == req.method())
            {
                // Create the bson request from the uri.
                request.set_child("command",
                        lj::bson::new_string(percent_decode(req.uri())));
            }
            else if (Http_request::Method::k_post == req.method())
            {
                // Create the bson request from the post body.
                std::multimap<std::string, std::string> params;
                std::string raw_params(reinterpret_cast<const char*>(req.body()),
                        req.content_length());
                process_params(params, raw_params);
                auto iter = params.find("cmd");
                if (params.end() != iter)
                {
                    request.set_child("command",
                            lj::bson::new_string(iter->second));
                }
                else
                {
                    request.set_child("command",
                            lj::bson::new_string(""));
                }
            }
            else
            {
                // Create the bson request from the request body (put).
                std::string cmd(reinterpret_cast<const char*>(req.body()),
                        req.content_length());
                request.set_child("command",
                        lj::bson::new_string(cmd));
            }

            // TODO support other command languages.
            log("Using [%s] for the command.").end(
                    lj::bson::as_string(request["command"]));
            request.set_child("language",
                    lj::bson::new_string("lua"));

            // Execute the command.
            pipe.sink() << request;
            safe_execute_stage(req.real_stage, swmr_xlator);

            // extract the response.
            lj::bson::Node response;
            pipe.source() >> response;

            // This should be updated to deal with exceptions, etc.
            std::string body(lj::bson::as_json_string(response));
            http_ios << HEADERS_SUCCESS
                    << HEADER_CONTENT_LENGTH << body.size()
                    << HEADER_LINE_ENDING << HEADER_LINE_ENDING
                    << body;
            http_ios.flush();
        }
        catch (lj::Exception& ex)
        {
            log("unexpected case: [%s]").end(ex);
            std::string body(ex.str());
            http_ios << HEADERS_SERVER_ERROR
                    << HEADER_CONTENT_LENGTH << body.size()
                    << HEADER_LINE_ENDING << HEADER_LINE_ENDING
                    << body;
            http_ios.flush();
        }

        // All http connections immediately disconnect.
        // TODO some keep alive stuff.
        log("Disconnecting.").end();
        return std::unique_ptr<Stage>(nullptr);
    }

    std::string Stage_http_adapt::name() const
    {
        std::string my_name("HTTP-Adapter");
        return my_name;
    }

    std::unique_ptr<logjam::Stage> Stage_http_adapt::clone() const
    {
        return std::unique_ptr<logjam::Stage>(new Stage_http_adapt(*this));
    }
};

