rm -fr libevhtp
tar -zxvf libevhtp.tar.gz
cd libevhtp/build
cmake ..
make
sudo make install
cd ..
