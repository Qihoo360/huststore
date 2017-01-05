sudo yum install -y libidn-devel

rm -fr curl-curl-7_50_2
tar -zxvf curl-curl-7_50_2.tar.gz
cd curl-curl-7_50_2
./buildconf
./configure --prefix=${prefix} --disable-ldap --disable-ldaps
make
sudo make install
cd ..

