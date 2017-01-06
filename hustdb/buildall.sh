#!/bin/bash
set -e
set -x

export prefix_3rd=/opt/huststore/3rd
export prefix_hustdb=/opt/huststore/hustdb


( cd ../third_party/ && bash -e build.sh )

( cd db && ./configure --prefix=${prefix_hustdb} --bindir=${prefix_hustdb} && make && sudo make install ;)
