rm -fr libunwind-1.1
tar zxvf libunwind-1.1.tar.gz
cd libunwind-1.1
./configure
make
sudo make install
cd ..

rm -fr gperftools-2.1
tar zxvf gperftools-2.1.tar.gz
cd gperftools-2.1
./configure
make
sudo make install
cd ..
