# vim: filetype=python
import re

APPNAME = 'logjammin'
VERSION = '0.2.0'
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cxx')
    opt.load('waf_unit_test')

def configure(conf):
    conf.load('compiler_cxx')
    conf.load('waf_unit_test')

    conf.check(
        header_name='openssl/ssl.h',
        lib=['ssl', 'crypto'],
        libpath=['/usr/local/lib', '/opt/local/lib', '/usr/lib'],
        includes=[
            '/usr/local/include',
            '/opt/local/include',
            '/usr/include'
            ],
        mandatory=True
    )

    conf.check(
        header_name='pthread.h',
        lib=['pthread'],
        libpath=['/usr/local/lib', '/opt/local/lib', '/usr/lib'],
        includes=[
            '/usr/local/include',
            '/opt/local/include',
            '/usr/include'
            ],
        mandatory=True
    )

    conf.check(
        header_name='cryptopp/cryptlib.h'
        ,lib=['cryptopp']
        ,libpath=['/usr/local/lib', '/opt/local/lib', '/usr/lib']
        ,includes=[
            '/usr/local/include'
            ,'/opt/local/include'
            ,'/usr/include'
        ]
        ,mandatory=True
    )

    conf.check(
        header_name='lua.hpp'
        ,lib=['lua5.1']
        ,libpath=['/usr/local/lib', '/opt/local/lib', '/usr/lib']
        ,includes=[
            '/usr/local/include'
            ,'/opt/local/include'
            ,'/usr/include'
            ,'/usr/include/lua5.1'
        ]
        ,mandatory=True
    )

    conf.check(
        header_name='v8.h'
        ,lib=['v8']
        ,libpath=['/usr/local/lib', '/opt/local/lib', '/usr/lib']
        ,includes=[
            '/usr/local/include'
            ,'/opt/local/include'
            ,'/usr/include'
        ]
        ,mandatory=True
    )
    
    conf.write_config_header('config.h')

def make_test_driver(node, test_pattern):
    drivernode = node.change_ext('_driver.h', '.cpp')
    cases = re.findall(test_pattern, node.read())
    output = ['#pragma once', '#include "testhelper.h"']
    for case in cases:
        output.append('void test' + case + '();')
    output.append('const Test_entry tests[] = {')
    for case in cases:
        output.append('  PREPARE_TEST(test' + case + '),')
    output.append('  {0, ""} };')
    code = '\n'.join(output)
    drivernode.write(code)

def build(bld):
    bld.load('compiler_cxx')
    bld.load('waf_unit_test')

    # setup unit test output.
    from waflib.Tools import waf_unit_test
    bld.add_post_fun(waf_unit_test.summary)

    # build the shared components
    bld.stlib(
        source = [
            'src/lj/Base64.cpp'
            ,'src/lj/Bson.cpp'
            ,'src/lj/Bson_parser.cpp'
            ,'src/lj/Log.cpp'
            ,'src/lj/Stopclock.cpp'
            ,'src/lj/Streambuf_pipe.cpp'
            ,'src/lj/Thread.cpp'
            ,'src/lj/Uuid.cpp'
            ,'src/scrypt/scrypt.cpp'
        ]
        ,target='lj'
        ,cxxflags = ['-O0', '-Wall', '-g', '-std=c++0x']
        ,includes = [
            './src'
        ]
        ,linkflags = ['-g']
        ,uselib = ['OPENSSL/SSL.H'
            ,'CRYPTOPP/CRYPTLIB.H']
    )

    bld.stlib(
        source = [
            'src/logjamd/Auth.cpp'
            ,'src/logjamd/Auth_local.cpp'
            ,'src/logjamd/Connection_secure.cpp'
            ,'src/logjamd/Server_secure.cpp'
            ,'src/logjamd/Stage_auth.cpp'
            ,'src/logjamd/Stage_execute.cpp'
            ,'src/logjamd/Stage_json_adapt.cpp'
            ,'src/logjamd/Stage_pre.cpp'
            ,'src/js/Command_language_js.cpp'
            ,'src/js/Bson.cpp'
            ,'src/lua/Bson.cpp'
            ,'src/lua/Command_language_lua.cpp'
            ,'src/lua/Document.cpp'
            ,'src/lua/Uuid.cpp'
        ]
        ,target='logjamserver'
        ,cxxflags = ['-O0', '-Wall', '-g', '-std=c++0x']
        ,includes = [
            './src'
        ]
        ,linkflags = ['-g']
        ,use = ['lj']
        ,uselib = ['OPENSSL/SSL.H', 'PTHREAD.H', 'LUA.HPP', 'V8.H']
    )

    bld.program(
        source = [
            'src/logjamd/logjamd.cpp'
        ]
        ,target='logjamd'
        ,cxxflags = ['-O0', '-Wall', '-g', '-std=c++0x']
        ,includes = [
            './src'
        ]
        ,linkflags = ['-g']
        ,use = ['lj', 'logjamserver']
    )

    # prepare the lua testing headers:
    lua_nodes = bld.path.ant_glob('test/lua/*.lua')
    lua_files = [
        '#include <string>',
        '#include <map>',
        'const std::map<std::string, std::string> lua_files {'
    ]
    for node in lua_nodes:
        lua_files.append(
            "    {\"" + node.abspath().rsplit('/', 1)[-1] + "\"," +
            "\"" + node.abspath() + "\"},"
        )
    lua_files.append("    {\"\", \"\"}};")
    lua_files.append("const std::string& path_for(const std::string& f){auto i=lua_files.find(f);return(i != lua_files.end()?i->second:lua_files.find(\"\")->second);}")
    bld.path.make_node("build/test/lua_files.h").write('\n'.join(lua_files));

    # preform the unit tests
    test_pattern = re.compile(r"void test(\w+)\s*\(\s*\)")
    test_nodes = bld.path.ant_glob('test/**/*.cpp')
    for node in test_nodes:
        make_test_driver(node, test_pattern)
        bld(
            features = ['cxx', 'cxxprogram', 'test']
            ,includes = [
                './test'
                ,'./src'
                ,'./build'
            ]
            ,source = [node]
            ,target = node.change_ext('')
            ,args = ['foo']
            ,use = ['lj', 'logjamserver']
            ,cxxflags = [
                '-O0'
                ,'-Wall'
                ,'-g'
                ,'-std=c++0x'
                ,'-fno-eliminate-unused-debug-types'
                ,'-fno-inline'
            ]
            ,linkflags = ['-g', '-pthread']
        )

