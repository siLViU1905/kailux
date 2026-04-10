@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=build
set GENERATORS_DIR=%BUILD_DIR%\build\generators

conan export scripts\imguizmo

if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)

echo [INFO] Conan install: DEBUG...
conan install . --output-folder=%BUILD_DIR% --build=missing ^
    -s build_type=Debug ^
    -s compiler.cppstd=23
    
if %errorlevel% neq 0 (
    echo [ERROR] Conan Debug install failed
    pause
    exit /b %errorlevel%
)

echo [INFO] Conan install: RELEASE...
conan install . --output-folder=%BUILD_DIR% --build=missing ^
    -s build_type=Release ^
    -s compiler.cppstd=23

if %errorlevel% neq 0 (
    echo [ERROR] Conan Release install failed
    pause
    exit /b %errorlevel%
)

mkdir %BUILD_DIR%
cd %BUILD_DIR%

cmake .. -G "Visual Studio 18 2026" ^
    -DCMAKE_TOOLCHAIN_FILE="%GENERATORS_DIR%\conan_toolchain.cmake" ^
    -DCMAKE_CXX_STANDARD=23 ^
    -DCMAKE_CXX_STANDARD_REQUIRED=ON

if %errorlevel% neq 0 (
    echo [ERROR] CMake config failed
    pause
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] sln file generated
pause