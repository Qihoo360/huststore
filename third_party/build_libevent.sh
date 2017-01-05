sudo yum install -y openssl-devel

rm -fr libevent-2.0.22-stable
tar -zxvf libevent-2.0.22-stable.tar.gz
cd libevent-2.0.22-stable
./configure --prefix=${prefix}
make
sudo make install
cd ..
