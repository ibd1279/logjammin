# vim: filetype=python
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
    conf.write_config_header('config.h')

def test(bld):
    #bld.recurse('test')
    pass

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
            ,'src/lj/Log.cpp'
            ,'src/lj/Stopclock.cpp'
            ,'src/lj/Thread.cpp'
            ,'src/lj/Uuid.cpp'
        ]
        ,target='lj'
        ,cxxflags = ['-O0', '-Wall', '-g', '-std=c++0x']
        ,includes = [
            './src'
        ]
        ,linkflags = ['-g']
        ,uselib = ['OPENSSL/SSL.H']
    )

    bld.program(
        source = [
            'src/logjamd/Connection_secure.cpp'
            ,'src/logjamd/Server_secure.cpp'
            ,'src/logjamd/Stage_auth.cpp'
            ,'src/logjamd/logjamd.cpp'
        ]
        ,target='logjamd'
        ,cxxflags = ['-O0', '-Wall', '-g', '-std=c++0x']
        ,includes = [
            './src'
        ]
        ,linkflags = ['-g']
        ,use = ['lj']
        ,uselib = ['OPENSSL/SSL.H', 'PTHREAD.H']
    )

    # preform the unit tests
    testnodes = bld.path.ant_glob('test/*.cpp')
    for node in testnodes:
        bld(
            features = ['cxx', 'cxxprogram', 'test']
            ,includes = [
                './test'
                ,'./src'
            ]
            ,source = [node]
            ,target = node.change_ext('')
            ,use = ['lj']
            ,cxxflags = ['-O0', '-Wall', '-g', '-std=c++0x']
            ,linkflags = ['-g', '-pthread']
        )

