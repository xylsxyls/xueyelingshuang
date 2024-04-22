::32位和64位需要分开编译，sln在protobuf-3.3.0_32\cmake\build\solution\protobuf.sln
cd ./protobuf-3.3.0_32
git clone -b release-1.7.0 https://github.com/google/googlemock.git gmock & cd gmock
git clone -b release-1.7.0 https://github.com/google/googletest.git gtest
cd .. & cd cmake & mkdir build & cd build & mkdir release & cd release
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../../../install ../..
cd .. & mkdir solution & cd solution
cmake -G "Visual Studio 12 2013" -DCMAKE_INSTALL_PREFIX=../../../../install ../..
::cmake -G "Visual Studio 12 2013 Win64" -DCMAKE_INSTALL_PREFIX=../../../../install ../..