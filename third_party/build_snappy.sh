test -d snappy-1.1.1 && rm -fr snappy-1.1.1
tar -zxvf snappy-1.1.1.tar.gz
cd snappy-1.1.1
sh ./configure --with-pic --enable-static --prefix=${prefix_3rd}
make -j`nproc`
make install
cd ..
