#!/bin/bash

conan export scripts/imguizmo

if [ -d "build" ]; then
    rm -rf build
fi

echo "[INFO] Conan install: DEBUG..."
conan install . --output-folder=build --build=missing \
    -s build_type=Debug \
    -s compiler.cppstd=23 \
    -c tools.system.package_manager:mode=install \
    -c tools.system.package_manager:sudo=True

if [ $? -ne 0 ]; then
    echo "[ERROR] Conan Debug install failed"
    exit 1
fi

mkdir -p build/debug
cd build/debug

cmake ../.. -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE="../build/Debug/generators/conan_toolchain.cmake" \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON

if [ $? -eq 0 ]; then
    echo "[SUCCESS] Debug config ready! Building..."
    make -j$(nproc)
else
    echo "[ERROR] Debug CMake config failed"
    exit 1
fi

cd ../..

echo "[INFO] Conan install: RELEASE..."
conan install . --output-folder=build --build=missing \
    -s build_type=Release \
    -s compiler.cppstd=23 \
    -c tools.system.package_manager:mode=install \
    -c tools.system.package_manager:sudo=True

if [ $? -ne 0 ]; then
    echo "[ERROR] Conan Release install failed"
    exit 1
fi

mkdir -p build/release
cd build/release

cmake ../.. -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="../build/Release/generators/conan_toolchain.cmake" \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON

if [ $? -eq 0 ]; then
    echo "[SUCCESS] Release config ready! Building..."
    make -j$(nproc)
else
    echo "[ERROR] Release CMake config failed"
    exit 1
fi