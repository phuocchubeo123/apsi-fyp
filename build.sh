cmake -B ../apsi2 -S . -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DAPSI_BUILD_CLI=ON
cmake --build ../apsi2
