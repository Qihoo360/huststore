sudo yum install -y libidn-devel

rm -fr curl-curl-7_50_2
tar -zxvf curl-curl-7_50_2.tar.gz
cd curl-curl-7_50_2 
./buildconf
./configure
make
cd ..

