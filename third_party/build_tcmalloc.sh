test -d libunwind-1.1 && rm -fr libunwind-1.1
tar zxvf libunwind-1.1.tar.gz
cd libunwind-1.1
./configure --prefix=${prefix_3rd}
make -j`nproc`
make install
cd ..

test -d gperftools-2.1 && rm -fr gperftools-2.1
tar zxvf gperftools-2.1.tar.gz
cd gperftools-2.1
./configure --enable-frame-pointers --prefix=${prefix_3rd}
make -j`nproc`
make install
cd ..
