sudo yum install openssl-devel -y

rm -fr libevent-2.0.22-stable
tar -zxvf libevent-2.0.22-stable.tar.gz
cd libevent-2.0.22-stable
./configure
make
sudo make install
cd ..
