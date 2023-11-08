cmake -B ../apsi -S . -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DAPSI_BUILD_CLI=ON
cmake --build ../apsi
