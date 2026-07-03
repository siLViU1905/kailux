#!/bin/bash
set -e

cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"

BUILD_TYPE=""
BUILD_DIR=""
DO_CLEAN=0

for arg in "$@"; do
    case "$arg" in
        debug|Debug)     BUILD_TYPE="Debug";   BUILD_DIR="debug" ;;
        release|Release) BUILD_TYPE="Release"; BUILD_DIR="release" ;;
        clean)           DO_CLEAN=1 ;;
        *) echo "[WARN] unknown arg: $arg" ;;
    esac
done

if [ -z "${BUILD_TYPE}" ]; then
    echo "[ERROR] Missing build type."
    echo "        You must specify 'debug' or 'release'."
    echo ""
    echo "        Usage:"
    echo "          ./linux_build.sh debug"
    echo "          ./linux_build.sh release"
    echo "          ./linux_build.sh debug clean     (wipe build/ first)"
    exit 1
fi

JOBS=$(nproc)
echo "[INFO] Build type: ${BUILD_TYPE}, jobs: ${JOBS}"

conan export scripts/imguizmo

if [ "$DO_CLEAN" -eq 1 ] && [ -d "build" ]; then
    echo "[INFO] Clean requested: removing build/"
    rm -rf build
fi

export CONAN_CMAKE_GENERATOR=Ninja

TOOLCHAIN="${PROJECT_ROOT}/build/build/${BUILD_TYPE}/generators/conan_toolchain.cmake"

if [ ! -f "${TOOLCHAIN}" ]; then
    echo "[INFO] Conan install: ${BUILD_TYPE}..."
    conan install . --output-folder=build --build=missing \
        -s build_type=${BUILD_TYPE} \
        -s compiler.cppstd=23 \
        -c tools.system.package_manager:mode=install \
        -c tools.system.package_manager:sudo=True \
        -c tools.cmake.cmaketoolchain:generator=Ninja
else
    echo "[INFO] Reusing existing conan deps for ${BUILD_TYPE}"
fi

if [ ! -f "${TOOLCHAIN}" ]; then
    echo "[ERROR] Toolchain not found at: ${TOOLCHAIN}"
    echo "        Conan install may have failed or used a different layout."
    echo "        Look under build/ for conan_toolchain.cmake:"
    find build -name conan_toolchain.cmake 2>/dev/null || true
    exit 1
fi

mkdir -p "build/${BUILD_DIR}"
cd "build/${BUILD_DIR}"

cmake "${PROJECT_ROOT}" -G "Ninja" \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "[INFO] Building with Ninja..."
cmake --build . -j "${JOBS}"

echo "[SUCCESS] ${BUILD_TYPE} build complete."