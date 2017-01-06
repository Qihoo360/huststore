#!/bin/bash
set -e
set -x

export prefix_3rd=/opt/huststore/3rd
export prefix_hustdb=/opt/huststore/hustdb
export prefix_hustdbsync=/opt/huststore/hustdbsync
export prefix_hustdbha=/opt/huststore/hustdbha


( cd ../third_party/ && bash -e build.sh )

( cd db   && ./configure --prefix=${prefix_hustdb}     --bindir=${prefix_hustdb}     && make -j$(nproc) && sudo make install ;)
( cd sync && ./configure --prefix=${prefix_hustdbsync} --bindir=${prefix_hustdbsync} && make -j$(nproc) && sudo make install ;)

export LD_LIBRARY_PATH=${prefix_3rd}/lib;
( cd ha/nginx && ./configure --with-cc-opt="-g3 -O0" --with-ld-opt="-lzlog -lpthread -lm -ldl -lcrypto" --prefix=${prefix_hustdbha} --add-module=src/addon ;)
