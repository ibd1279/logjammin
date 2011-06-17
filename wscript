# vim: filetype=python
top = '.'
out = 'build'

def options(ctx):
    ctx.tool_options('compiler_cxx')
    ctx.add_option('--dbdir', action='store', default='/var/db/logjam',
        help='Directory to store DB information. [default: \'/var/db/logjam\']', dest='dbdir')

def configure(ctx):
    import Options

    #Set the DB directory.
    ctx.env.DBDIR = Options.options.dbdir
    ctx.define('DBDIR', Options.options.dbdir)
    ctx.env.vnum = '0.1.0'

    #Set the compiler 
    ctx.check_tool('compiler_cxx')
    ctx.check_cxx(
        header_name='histedit.h',
        lib=['edit', 'curses'],
        libpath = ['/usr/lib'],
        includes='/usr/include/',
        define_name='HAVE_EDITLINE'
    )
    ctx.check_cxx(
        header_name='tcutil.h',
        lib=['tokyocabinet'],
        libpath=['/usr/local/lib', '/opt/local/lib/'],
        includes=['/usr/local/include', '/opt/local/include/'],
        mandatory=True
    )
    ctx.check_cxx(
        header_name='lua.h',
        lib=['lua5.1'],
        libpath=['/usr/local/lib', '/opt/local/lib/', '/usr/lib'],
        includes=[
            '/usr/local/include',
            '/opt/local/include',
            '/usr/include',
            '/usr/local/include/lua5.1',
            '/opt/local/include/lua5.1',
            '/usr/include/lua5.1'
        ],
        mandatory=True
    )
    ctx.check(
        header_name='openssl/ssl.h',
        lib=['ssl', 'crypto'],
        libpath=['/usr/local/lib', '/opt/local/lib', '/usr/lib'],
        includes=[
            '/usr/local/include',
            '/opt/local/include',
            '/usr/include',
            '/usr/local/include/lua5.1',
            '/opt/local/include/lua5.1',
            '/usr/include/lua5.1'
        ],
        mandatory=True
    )
    ctx.check_cxx(
        header_name='sys/types.h',
        define_name='HAVE_SYS_TYPES_H'
    )
    ctx.write_config_header('config.h')

def build(ctx):
    logjamd = ctx(
        features = ['cxx', 'cprogram']
        ,source = [
                'src/lj/Base64.cpp'
                ,'src/lj/Bson.cpp'
                ,'src/lj/Log.cpp'
                ,'src/lj/Stopclock.cpp'
                ,'src/lj/Uuid.cpp'
                ]
        ,target = 'logjamd'
        ,vnum = ctx.env.vnum
        ,includes = [
                '.'
                ,'./src'
                ,'/usr/local/include'
                ,'/opt/local/include'
                ,'/usr/include']
        ,cxxflags = ['-O0', '-Wall', '-g', '-std=c++0x']
        ,libpath = ['/usr/local/lib/', '/opt/local/lib/', '/usr/lib']
        ,linkflags = ['-g']
        ,uselib = ['TCUTIL.H', 'DYSTOPIA.H', 'LUA.H', 'OPENSSL/SSL.H']
    )

