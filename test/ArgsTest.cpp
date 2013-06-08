/*!
 \file test/ArgsTest.cpp
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

#include "testhelper.h"
#include "lj/Args.h"
#include "test/ArgsTest_driver.h"

void testParseFlags()
{
    // Create the fake arguments.
    int argc = 3;
    const char* argv[] = { "app", "--flag1", "--flag2"  };
    
    // Create the arguments data.
    lj::ArgParser parser(argc, argv);
    lj::FlagArg flag1("--flag1", "-1", "The first flag", parser);
    lj::FlagArg flag2("--flag2", "-2", "The second flag", parser);
    lj::FlagArg flag3("--flag3", "-3", "The third flag", parser);
    parser.parse();

    // test that things are set properly.
    TEST_ASSERT(flag1.present());
    TEST_ASSERT(flag1.boolean());
    TEST_ASSERT(flag2.present());
    TEST_ASSERT(flag2.boolean());
    TEST_ASSERT(!flag3.present());
    TEST_ASSERT(!flag3.boolean());
}

void testParseSettings()
{
    // Create the fake arguments.
    int argc = 8;
    const char* argv[] = { "app", "--setting1", "zot", "-2", "bar", "--setting3=baz", "--setting1", "foo" };
    //
    lj::ArgParser parser(argc, argv);
    lj::SettingArg setting1("--setting1", "-1", "The first flag", "", parser);
    lj::SettingArg setting2("--setting2", "-2", "The second flag", "zot", parser);
    lj::SettingArg setting3("--setting3", "-3", "The third flag", "", parser);
    lj::SettingArg setting4("--setting4", "-4", "The fourth flag", "biff", parser);
    parser.parse();

    TEST_ASSERT(setting1.present());
    TEST_ASSERT(setting1.str().compare("foo") == 0);
    TEST_ASSERT(setting2.present());
    TEST_ASSERT(setting2.str().compare("bar") == 0);
    TEST_ASSERT(setting3.present());
    TEST_ASSERT(setting3.str().compare("baz") == 0);
    TEST_ASSERT(!setting4.present());
    TEST_ASSERT(setting4.str().compare("biff") == 0);
}

void testParseList()
{
    int argc = 11;
    const char* argv[] = { "app", "--setting1", "a", "--setting1", "b", "--setting2=1", "--setting2", "2", "--setting1=c", "--setting2", "3" };
    lj::ArgParser parser(argc, argv);
    lj::ListArg list1("--setting1", "-1", "The first flag", std::list<std::string>{}, parser);
    lj::ListArg list2("--setting2", "-2", "The second flag", std::list<std::string>{"4", "5"}, parser);
    lj::ListArg list3("--setting3", "-3", "The third flag", std::list<std::string>{"y", "z"}, parser);
    parser.parse();

    TEST_ASSERT(list1.present());
    TEST_ASSERT(list1.list().size() == 3);
    char h = 'a';
    for(auto iter = list1.list().begin(); iter != list1.list().end(); ++h, ++iter)
    {
        TEST_ASSERT((*iter).front() == h);
    }

    TEST_ASSERT(list2.present());
    TEST_ASSERT(list2.list().size() == 3);
    h = '1';
    for(auto iter = list2.list().begin(); iter != list2.list().end(); ++h, ++iter)
    {
        TEST_ASSERT((*iter).front() == h);
    }

    TEST_ASSERT(!list3.present());
    TEST_ASSERT(list3.list().size() == 2);
    h = 'y';
    for(auto iter = list3.list().begin(); iter != list3.list().end(); ++h, ++iter)
    {
        TEST_ASSERT((*iter).front() == h);
    }
}

void testUnknownFlag()
{
    int argc = 3;
    const char* argv[] = { "app", "--setting1", "a" };

    try
    {
        lj::ArgParser parser(argc, argv);
        parser.parse();
        TEST_FAILED("Should have thrown an exception.");
    }
    catch(lj::Exception& ex)
    {
        std::string expected("Invalid Argument Exception: app doesn't know how to deal with --setting1");
        TEST_ASSERT(expected.compare(ex.what()) == 0);
    }
}

void testRequired()
{
    int argc = 1;
    const char* argv[] = { "app" };

    try
    {
        lj::ArgParser parser(argc, argv);
        lj::FlagArg flag1("--required-flag", "", "required flag", parser);
        flag1.required(true);
        parser.parse();
        TEST_FAILED("Should have thrown an exception.");
    }
    catch(lj::Exception& ex)
    {
        std::string expected("Missing Argument Exception: --required-flag is a required, but not present.");
        TEST_ASSERT(expected.compare(ex.what()) == 0);
    }
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::Args", tests);
}

