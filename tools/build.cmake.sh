
version=3.17
build=2
mkdir ~/temp
cd ~/temp
curl https://cmake.org/files/v$version/cmake-$version.$build.tar.gz --output ./cmake-$version.$build.tar.gz
tar -xzvf cmake-$version.$build.tar.gz
cd cmake-$version.$build/
./bootstrap
make -j$(nproc)
make install
cmake --version
