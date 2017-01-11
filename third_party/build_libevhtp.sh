test -d libevhtp && rm -fr libevhtp
tar -zxvf libevhtp.tar.gz
cd libevhtp/build
cmake -DCMAKE_INSTALL_PREFIX:PATH=${prefix_3rd} ..
make -j`nproc`
make install
cd ../../
