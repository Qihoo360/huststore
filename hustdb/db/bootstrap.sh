#!/bin/bash
set -e
set -x

touch NEWS README ChangeLog AUTHORS COPYING INSTALL

aclocal
autoconf
autoheader
libtoolize --automake --copy --force
automake --add-missing --copy
