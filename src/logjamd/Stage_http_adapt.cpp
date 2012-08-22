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
#include "lj/Log.h"
#include "lj/Base64.h"

#include <map>
#include <iostream>

namespace
{
    const std::string HTTP_VERSION_1_0("HTTP/1.0");
    const std::string HTTP_VERSION_1_1("HTTP/1.1");
    const std::string REQUIRE_AUTH_PREFIX("~/");
    const std::string HEADER_LINE_ENDING("\r\n");
    const std::string HEADER_CONTENT_LENGTH("Content-Length: ");
    const std::string HEADERS_AUTH_REQUIRED("HTTP/1.1 401 Unauthorized\r\nServer: Logjamd\r\nContent-Type: application/json; charset=\"UTF-8\"\r\nWWW-Authenticate: Basic realm=\"Secure Command Execution\"\r\n");
    const std::string HEADERS_FORBIDDEN("HTTP/1.0 403 Forbidden\r\nServer: Logjamd\r\nContent-Type: application/json; charset=\"UTF-8\"\r\n");
    const std::string HEADERS_SERVER_ERROR("HTTP/1.0 500 Internal Server Error\r\nServer: Logjamd\r\nContent-Type: application/json; charset=\"UTF-8\"\r\n");
    const std::string HEADERS_SUCCESS("HTTP/1.0 200 OK\r\nContent-Type: application/json; charset=\"UTF-8\"\r\n");

    //! Simplified method for getting lines from the http connection.
    inline size_t get_http_line(std::iostream& input_stream,
            std::string& line)
    {
        bool loop = true;
        line.erase();
        while (loop)
        {
            // Try to get the header from the stream.
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

            // Deal with continuing lines.
            size = buffer.size();
            if (size > 0)
            {
                if (buffer[size - 1] != '\\')
                {
                    loop = false;
                }
                else
                {
                    buffer.erase(size - 1);
                }
            }
            else
            {
                loop = false;
            }

            // Add the buffer to the header line.
            line.append(buffer);
        }
        return line.size();
    }

    inline void header_to_key_value(const std::string& header,
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
};

namespace logjamd
{
    Stage_http_adapt::Stage_http_adapt(Connection* connection) :
            Stage_adapt(connection),
            real_stage_(new_auth_stage())
    {
    }

    Stage_http_adapt::~Stage_http_adapt()
    {
    }

    Stage* Stage_http_adapt::logic()
    {
        // Read the requested path.
        try
        {
            std::string cmd;
            std::multimap<std::string, std::string> headers;
            size_t cmd_length = get_http_line(conn()->io(), cmd);

            // trim off the http version
            log("Starting with [%s] as the command. %d as the length").end(cmd, cmd_length);
            cmd_length = cmd_length > 9 ? cmd_length - 9 : 0;
            cmd.erase(cmd_length);

            // Read all the request headers.
            std::string buffer;
            while (get_http_line(conn()->io(), buffer) > 0)
            {
                std::string key;
                std::string value;
                header_to_key_value(buffer, key, value);
                headers.insert(std::pair<std::string, std::string>(key, value));
                log("Received [%s]: [%s]").end(key, value);
            }
            log("done with the headers").end();

            // deal with requested authorization.
            lj::bson::Node auth_request;
            bool can_retry = false;
            if (0 == cmd.compare(0, REQUIRE_AUTH_PREFIX.size(),
                        REQUIRE_AUTH_PREFIX))
            {
                // Authentication is required. 
                log("Login required.").end();

                // Look for authentication headers
                auto auth_header = headers.find("Authorization");

                // Deal with missing authentication information
                if (auth_header == headers.end())
                {
                    std::string body("Authentication information required.");
                    conn()->io() << HEADERS_AUTH_REQUIRED;
                    conn()->io() << HEADER_CONTENT_LENGTH << body.size();
                    conn()->io() << HEADER_LINE_ENDING << HEADER_LINE_ENDING;
                    conn()->io() << body << std::endl;
                    conn()->io().flush();
                    return nullptr;
                }

                // setup the auth_request object.
                std::string encoded_user_data(auth_header->second.substr(6));
                size_t sz;
                uint8_t* data = lj::base64_decode(encoded_user_data, &sz);
                std::string login_data(reinterpret_cast<char*>(data), sz);
                sz = login_data.find_first_of(':');
                auth_request.set_child("method",
                        lj::bson::new_uuid(lj::Uuid(k_auth_method,
                                "password_hash",
                                13)));
                auth_request.set_child("provider",
                        lj::bson::new_uuid(lj::Uuid(k_auth_provider,
                                "local",
                                5)));
                auth_request.set_child("data/login",
                        lj::bson::new_string(login_data.substr(0, sz)));
                auth_request.set_child("data/password",
                        lj::bson::new_string(login_data.substr(sz + 1)));
                can_retry = true;
                
                // remove the auth request part of the command
                cmd = cmd.substr(2);
            }
            else
            {
                // If the conection is insecure, use default login.
                log("Using insecure adapter authentication.").end();

                auth_request.set_child("method",
                        lj::bson::new_uuid(lj::Uuid(k_auth_method,
                                "password_hash",
                                13)));
                auth_request.set_child("provider",
                        lj::bson::new_uuid(lj::Uuid(k_auth_provider,
                                "local",
                                5)));
                auth_request.set_child("data/login",
                        lj::bson::new_string(k_user_login_json));
                auth_request.set_child("data/password",
                        lj::bson::new_string(k_user_password_json));
            }

            // Log into the system.
            log("Authenticating user [%s].").end(
                    lj::bson::as_string(auth_request["data/login"]));
            pipe().sink() << auth_request;
            std::unique_ptr<Stage> next_stage(real_stage_->logic());

            // Deal with any stages that return themselves. unique_ptr does
            // not guard against self resets.
            if (real_stage_ == next_stage)
            {
                next_stage.release();
            }
            else
            {
                real_stage_.reset(next_stage.release());
            }

            // extract the system response.
            lj::bson::Node auth_response;
            pipe().source() >> auth_response;

            // Handle login failures.
            if (lj::bson::as_boolean(auth_response["success"]) == false
                    || faux_connection().user() == nullptr
                    || real_stage_ == nullptr)
            {
                log("Login unsuccessful.").end();
                std::string body(lj::bson::as_pretty_json(auth_response));
                if (can_retry)
                {
                    // If they provided credentials, they can try again.
                    conn()->io() << HEADERS_AUTH_REQUIRED;
                }
                else
                {
                    // don't bother retrying, no credentials will get you in.
                    conn()->io() << HEADERS_FORBIDDEN;
                }
                conn()->io() << HEADER_CONTENT_LENGTH << body.size();
                conn()->io() << HEADER_LINE_ENDING << HEADER_LINE_ENDING;
                conn()->io() << body << std::endl;
                conn()->io().flush();
                return nullptr;
            }

            // we got here on a successful login.
            log("Login successful.").end();

            // Create the bson request.
            log("Using [%s] for the command.").end(cmd);
            lj::bson::Node request;
            request.set_child("command",
                    lj::bson::new_string(cmd));
            request.set_child("language",
                    lj::bson::new_string(language()));

            // Process the bson command.
            pipe().sink() << request;
            next_stage.reset(real_stage_->logic());

            // Deal with any stages that return themselves. unique_ptr does
            // not guard against self resets.
            if (real_stage_ == next_stage)
            {
                next_stage.release();
            }
            else
            {
                real_stage_.reset(next_stage.release());
            }

            // Extract the response from the faux connection.
            lj::bson::Node response;
            pipe().source() >> response;

            // This should be updated to deal with exceptions, etc.
            std::string body(lj::bson::as_pretty_json(response));
            conn()->io() << HEADERS_SUCCESS;
            conn()->io() << HEADER_CONTENT_LENGTH << body.size();
            conn()->io() << HEADER_LINE_ENDING << HEADER_LINE_ENDING;
            conn()->io() << body << std::endl;
            conn()->io().flush();
        }
        catch (lj::Exception& ex)
        {
            log("unexpected case: [%s]").end(ex);
            std::string body(ex.str());
            conn()->io() << HEADERS_SERVER_ERROR;
            conn()->io() << HEADER_CONTENT_LENGTH << body.size();
            conn()->io() << HEADER_LINE_ENDING << HEADER_LINE_ENDING;
            conn()->io() << body << std::endl;
            conn()->io().flush();
        }

        // All http connections immediately disconnect.
        log("Disconnecting.").end();
        return nullptr;
    }

    std::string Stage_http_adapt::name()
    {
        std::string my_name("HTTP-Adapter-");
        return my_name + real_stage_->name();
    }
};

