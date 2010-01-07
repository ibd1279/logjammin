#!/bin/sh

mkdir -p /var/db/logjammin/static
cp *.json *.html /var/db/logjammin/
cp *.js *.css /var/db/logjammin/static
cp build/Debug/logjammin /Library/WebServer/CGI-Executables/logjammin
