#!/bin/bash

set -e

export PATH=/usr/local/bin:/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/sbin:/home/search/bin

bash -e ./build_snappy.sh       || { echo "Build snappy   failed." ; false }
bash -e ./build_leveldb.sh      || { echo "Build leveldb  failed." ; false }
bash -e ./build_cmake.sh        || { echo "Build cmake    failed." ; false }
bash -e ./build_libevent.sh     || { echo "Build libevent failed." ; false }
bash -e ./build_libevhtp.sh     || { echo "Build libevhtp failed." ; false }
bash -e ./build_tcmalloc.sh     || { echo "Build tcmalloc failed." ; false }
bash -e ./build_libcurl.sh      || { echo "Build libcurl  failed." ; false }
bash -e ./build_libzlog.sh      || { echo "Build libzlog  failed." ; false }
