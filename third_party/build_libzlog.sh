rm -fr zlog-latest-stable
tar -zxvf zlog-latest-stable.tar.gz
cd zlog-latest-stable
make -j`nproc`
sudo make PREFIX=${prefix_3rd} install
cd ..

