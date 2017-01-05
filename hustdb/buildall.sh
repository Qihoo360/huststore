#!/bin/bash
set -e
set -x

export prefix=/opt/huststore/3rd


( cd ../third_party/ && bash -e build.sh )

./confiure --prefix=${prefix}
make
sudo make install
