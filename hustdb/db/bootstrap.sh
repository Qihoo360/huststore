set -e
set -x

cp -v  /usr/share/automake-1.*/{depcomp,compile}  ./

aclocal 
autoconf
autoheader
libtoolize --automake --copy --debug --force 
automake --add-missing

