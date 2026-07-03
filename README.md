# Kailux

A lightweight, modern C++ rendering engine.

## Requirements
- Vulkan 1.3+
- conan
- cmake
- c++23 compatible compiler

## Building the Project

## Windows
**In the bat file the generator is set to "Visual Studio 18 2026". If you don't have it, use your Visual Studio version (eg: "Visual Studio 17 2022").**

Run the provided batch file with a build type:

```bat
scripts\windows_build.bat debug
scripts\windows_build.bat release
scripts\windows_build.bat debug clean
```

In the build folder you will find the sln file — open it with Visual Studio and build.

## Linux

Run the provided shell script with a build type:

```bash
scripts/linux_build.sh debug
scripts/linux_build.sh release
scripts/linux_build.sh debug clean
```

In the build folder (`build/[debug|release]/app`) you will find the executable `kailux_application`.