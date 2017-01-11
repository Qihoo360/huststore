tar -zxf leveldb.tar.gz
cd leveldb
chmod a+x build_detect_platform
make clean
make -j`nproc`
test -e ${prefix_3rd}/lib || mkdir -p ${prefix_3rd}/lib
install -v libleveldb.a libleveldb.so*  ${prefix_3rd}/lib/
cp  -rv  include/leveldb  ${prefix_3rd}/include/
cd ..
