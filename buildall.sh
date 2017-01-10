#!/bin/bash
set -e
set -x


export prefix_top=/opt/huststore/

export prefix_3rd=${prefix_top}/3rd
export prefix_hustdb=${prefix_top}/hustdb
export prefix_hustdbsync=${prefix_top}/hustdbsync
export prefix_hustdbha=${prefix_top}/hustdbha
export prefix_hustmq=${prefix_top}/hustmq
export prefix_hustmqha=${prefix_top}/hustmqha


( cd ../third_party/ && bash -e build.sh )

( cd db       && ./bootstrap.sh && ./configure --prefix=${prefix_hustdb}     --bindir=${prefix_hustdb}     && make -j$(nproc) && sudo make install ;)
( cd sync     && ./bootstrap.sh && ./configure --prefix=${prefix_hustdbsync} --bindir=${prefix_hustdbsync} && make -j$(nproc) && sudo make install ;)
( cd ha/nginx && ./configure \
  --with-cc-opt="-g3 -O0 -I${prefix_3rd}/include" \
  --with-ld-opt="-L${prefix_3rd}/lib -lzlog -lpthread -lm -ldl -lcrypto" \
  --prefix=${prefix_hustdbha} --add-module=src/addon && make -j$(nproc) && sudo make install ;)

