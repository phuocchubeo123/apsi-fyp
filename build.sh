cmake -B ../v2 -S . -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DAPSI_BUILD_CLI=ON
cmake --build ../v2
