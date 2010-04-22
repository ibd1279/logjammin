top = '.'
out = 'build'

def set_options(ctx):
    ctx.tool_options('compiler_cxx')

def configure(ctx):
    ctx.check_tool('compiler_cxx')
    ctx.check_cxx(
        header_name='histedit.h',
        lib=['edit', 'curses'],
        libpath = ['/usr/lib'],
        includes='/usr/include/',
        define_name='HAVE_EDITLINE'
    )
    ctx.write_config_header('config.h')
    

def build(ctx):
    t = ctx(
        features = ['cxx', 'cprogram'],
        source = 'main.cpp Tokyo.cpp Document.cpp Storage.cpp',
        target = 'logjam',
        vnum = '0.1.0',
        includes = ['.', '/usr/local/include/', '/opt/local/include/', '/usr/include'],
        cxxflags = ['-O0', '-Wall'],
        lib = ['tokyocabinet', 'lua', 'tokyodystopia'],
        libpath = ['/usr/local/lib/', '/opt/local/lib/', '/usr/lib'],
        linkflags = ['-g'],
        uselib = ['HISTEDIT.H']
    )
    t
