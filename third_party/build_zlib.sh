test -d zlib-1.2.11 && rm -fr zlib-1.2.11
tar -zxvf zlib-1.2.11.tar.gz
cd zlib-1.2.11
./configure --prefix=${prefix_3rd}
make -j`nproc`
make install
cd ..
