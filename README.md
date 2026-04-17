# Kailux

A lightweight, modern C++ rendering engine.

## Requirements
- Vulkan 1.3+
- conan
- cmake
- c++23 compatible compiler

## Building the Project

## Windows
**In the bat file the generator is set to "Visual Studio 18 2026" if you dont have it use your Visual Studio version(eg: "Visual Studio 17 2022")**

The only thing is to run the provided batch file:

```bat
scripts/windows_build.bat
```
In the build folder you will find the sln file, open it with Visual Studio and build

## Linux

```bat
scripts/linux_build.sh
```
In the build folder([debug/release]/app) you will find the executable 'kailux_application'