test -d cmake-3.5.0 && rm -fr cmake-3.5.0
tar -zxvf cmake-3.5.0.tar.gz
cd cmake-3.5.0
chmod a+x ./bootstrap
./bootstrap --prefix=${prefix_3rd}
make -j`nproc`
make install
cd ..
