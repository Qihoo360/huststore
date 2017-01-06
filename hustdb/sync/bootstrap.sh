#!/bin/bash
set -e
set -x


touch NEWS README ChangeLog  AUTHORS
cp -v  /usr/share/automake-1.*/{depcomp,compile}  ./

aclocal
autoconf
autoheader
libtoolize --automake --copy --debug --force
automake --add-missing
