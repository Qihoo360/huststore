tar -zxf leveldb.tar.gz
cd leveldb
chmod a+x build_detect_platform
make clean
make
sudo install -v libleveldb.a libleveldb.so*  ${prefix}/lib/
cd ..
