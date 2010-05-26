top = '.'
out = 'build'

def set_options(ctx):
    ctx.tool_options('compiler_cxx')
    ctx.add_option('--dbdir', action='store', default='/var/db/logjam',
        help='Directory to store DB information. [default: \'/var/db/logjam\']', dest='dbdir')
    ctx.add_option('--dbdir-make', action='store_true', default=False,
        help='Create the DBDirectory during install. [default: False]', dest='dbdirmake')

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
        header_name='dystopia.h',
        lib=['tokyodystopia'],
        libpath=['/usr/local/lib', '/opt/local/lib/'],
        includes=['/usr/local/include', '/opt/local/include/'],
        mandatory=True
    )
    ctx.check_cxx(
        header_name='tcutil.h',
        lib=['tokyocabinet'],
        libpath=['/usr/local/lib', '/opt/local/lib/'],
        includes=['/usr/local/include', '/opt/local/include/'],
        mandatory=True
    )
    ctx.write_config_header('config.h')

def make_data_dir(ctx):
    import Utils, Options
    if Options.options.dbdirmake:
        Utils.exec_command('mkdir -p %s' % ctx.env.DBDIR)

def build(ctx):
    ctx.add_post_fun(make_data_dir)

    logjamd = ctx(
        features = ['cxx', 'cprogram']
        ,source = [
                'src/lj/All_record_set.cpp'
                ,'src/lj/Bson.cpp'
                ,'src/lj/Logger.cpp'
                ,'src/lj/Record_set.cpp'
                ,'src/lj/Sockets.cpp'
                ,'src/lj/Standard_record_set.cpp'
                ,'src/lj/Storage.cpp'
                ,'src/lj/Storage_factory.cpp'
                ,'src/lj/Time_tracker.cpp'
                ,'src/logjamd/Lua_bson.cpp'
                ,'src/logjamd/Lua_record_set.cpp'
                ,'src/logjamd/Lua_storage.cpp'
                ,'src/logjamd/logjamd.cpp'
                ,'src/logjamd/logjamd_lua.cpp'
                ,'src/logjamd/logjamd_net.cpp'
                ,'src/tokyo/Tokyo.cpp'
                ]
        ,target = 'logjamd'
        ,vnum = ctx.env.vnum
        ,includes = [
                '.'
                ,'./src'
                ,'/usr/local/include'
                ,'/opt/local/include'
                ,'/usr/include']
        ,cxxflags = ['-O0', '-Wall', '-g']
        ,lib = ['lua']
        ,libpath = ['/usr/local/lib/', '/opt/local/lib/', '/usr/lib']
        ,linkflags = ['-g']
        ,uselib = ['TCUTIL.H', 'DYSTOPIA.H']
    )

    logjam = ctx(
        features = ['cxx', 'cprogram']
        ,source = [
                'src/lj/Logger.cpp'
                ,'src/lj/Sockets.cpp'
                ,'src/lj/Client.cpp'
                ,'src/lj/Bson.cpp'
                ,'src/logjam/logjam.cpp'
                ]
        ,target = 'logjam'
        ,vnum = ctx.env.vnum
        ,includes = [
                '.'
                ,'./src'
                ,'/usr/local/include'
                ,'/opt/local/include'
                ,'/usr/include']
        ,cxxflags = ['-O0', '-Wall', '-g']
        ,lib = ['lua']
        ,libpath = ['/usr/local/lib/', '/opt/local/lib/', '/usr/lib']
        ,linkflags = ['-g']
        ,uselib = ['HISTEDIT.H']
    )
