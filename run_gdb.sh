export #!/bin/sh
export HTTP_HOST=localhost
export HTTP_USER_AGENT='Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.5; en-US; rv:1.9.1.1) Gecko/20090715 Firefox/3.5.1'
export HTTP_ACCEPT='text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8'
export HTTP_ACCEPT_LANGUAGE=en-us,en;q=0.5
export HTTP_ACCEPT_ENCODING=gzip,deflate
export HTTP_ACCEPT_CHARSET=ISO-8859-1,utf-8;q=0.7,*;q=0.7
export HTTP_KEEP_ALIVE=300
export HTTP_CONNECTION=keep-alive
export HTTP_REFERER=http://localhost/cgi-bin/logjammin/
export HTTP_CACHE_CONTROL=max-age=0
export PATH=/usr/bin:/bin:/usr/sbin:/sbin
export SERVER_SIGNATURE=
export SERVER_SOFTWARE='Apache/2.2.11 (Unix) mod_ssl/2.2.11 OpenSSL/0.9.7l DAV/2 PHP/5.2.8'
export SERVER_NAME=localhost
export SERVER_ADDR=::1
export SERVER_PORT=80
export REMOTE_ADDR=::1
export DOCUMENT_ROOT=/Library/WebServer/Documents
export SERVER_ADMIN=you@example.com
export SCRIPT_FILENAME=/Library/WebServer/CGI-Executables/logjammin
export REMOTE_PORT=61703
export GATEWAY_INTERFACE=CGI/1.1
export SERVER_PROTOCOL=HTTP/1.1
export REQUEST_METHOD=GET
export QUERY_STRING=''
export REQUEST_URI='/cgi-bin/logjammin/role-list'
export SCRIPT_NAME=/cgi-bin/logjammin
export PATH_INFO=/role-list
export HTTP_COOKIE='lj_user_login=http://openid.aol.com/ibd1279; lj_user_cookie=must_set_env_var_GDB_MODE'
export GDB_MODE=true
export PATH_TRANSLATED='/Library/WebServer/Documents/'
export MallocStackLoggingNoCompact=true

gdb ./build/Debug/logjammin
