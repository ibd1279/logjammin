/*!
 \file test/lua/luaTest.cpp
 Copyright (c) 2014, Jason Watson
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

#include "testhelper.h"
#include "lj/Bson.h"
#include "lua/Command_language_lua.h"
#include "logjamd/mock_server.h"
#include "test/lua/luaTest_driver.h"

#include "test/lua_files.h"
#include <ios>
#include <fstream>

template <class T>
class Invoke_script_test
{
public:
    Invoke_script_test() :
            request_(),
            env_(),
            language_()
    {
    }

    ~Invoke_script_test()
    {
    }

    inline lj::bson::Node& request()
    {
        return request_;
    }

    inline Mock_env& env()
    {
        return env_;
    }

    inline T& language()
    {
        return language_;
    }

    lj::bson::Node perform(const std::string& path)
    {
        request_.set_child("command",
                lj::bson::new_string(read_file(path)));

        // This response object should come from a
        // connection or a stage or something. it shouldn't be built here.
        lj::bson::Node response;
        response.set_child("success", lj::bson::new_boolean(true));
        response.set_child("message", lj::bson::new_string("ok"));
        response.set_child("output", lj::bson::new_array());
        language_.perform(*(env().swimmer), request(), response);

        std::cout << lj::bson::as_json_string(response) << std::endl;

        TEST_ASSERT_MSG(lj::bson::as_string(response.nav("message")),
                lj::bson::as_boolean(response.nav("success")));

        return response;
    }
private:
    lj::bson::Node request_;
    Mock_env env_;
    T language_;

    std::string read_file(const std::string& filename)
    {
        char* buffer;
        std::ifstream is(filename,
                std::ios_base::binary | std::ios_base::in);
        is.seekg(0, std::ios_base::end);
        int length = is.tellg();
        is.seekg(0, std::ios_base::beg);

        buffer = new char[length];
        int loc = 0;
        while(loc < length)
        {
            is.read(buffer, length - loc);
            loc = is.tellg();
        }
        is.close();
        std::string tmp(buffer, length);
        delete buffer;
        return tmp;
    }
};

void testBasicCommands()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    lj::bson::Node response(
            harness.perform(path_for("Command_language_luaTest.lua")));
}

void testBson()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    lj::bson::Node response(
            harness.perform(path_for("BsonTest.lua")));
}

void testUuid()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    lj::bson::Node response(
            harness.perform(path_for("UuidTest.lua")));
}

void testDocument()
{
    Invoke_script_test<lua::Command_language_lua> harness;
    lj::bson::Node response(
            harness.perform(path_for("DocumentTest.lua")));
}

int main(int argc, char** argv)
{
    return Test_util::runner("lua::Command_language_lua", tests);
}

