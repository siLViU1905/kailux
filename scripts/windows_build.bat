@echo off
setlocal enabledelayedexpansion

cd ..

set BUILD_TYPE=
set DO_CLEAN=0

:parse_args
if "%~1"=="" goto after_args
if /i "%~1"=="debug"   set BUILD_TYPE=Debug
if /i "%~1"=="release" set BUILD_TYPE=Release
if /i "%~1"=="clean"   set DO_CLEAN=1
shift
goto parse_args
:after_args

if "%BUILD_TYPE%"=="" (
    echo [ERROR] Missing build type.
    echo         You must specify 'debug' or 'release'.
    echo.
    echo         Usage:
    echo           windows_build.bat debug
    echo           windows_build.bat release
    echo           windows_build.bat debug clean     ^(wipe build\ first^)
    pause
    exit /b 1
)

set BUILD_DIR=build
set GENERATORS_DIR=%BUILD_DIR%\build\%BUILD_TYPE%\generators
set TOOLCHAIN=%GENERATORS_DIR%\conan_toolchain.cmake

echo [INFO] Build type: %BUILD_TYPE%

conan export scripts\imguizmo

if %DO_CLEAN%==1 (
    if exist %BUILD_DIR% (
        echo [INFO] Clean requested: removing %BUILD_DIR%
        rmdir /s /q %BUILD_DIR%
    )
)

if not exist "%TOOLCHAIN%" (
    echo [INFO] Conan install: %BUILD_TYPE%...
    conan install . --output-folder=%BUILD_DIR% --build=missing ^
        -s build_type=%BUILD_TYPE% ^
        -s compiler.cppstd=23
    if !errorlevel! neq 0 (
        echo [ERROR] Conan %BUILD_TYPE% install failed
        pause
        exit /b !errorlevel!
    )
) else (
    echo [INFO] Reusing existing conan deps for %BUILD_TYPE%
)

if not exist "%TOOLCHAIN%" (
    echo [ERROR] Toolchain not found at: %TOOLCHAIN%
    echo         Conan install may have failed or used a different layout.
    pause
    exit /b 1
)

mkdir %BUILD_DIR% 2>nul
cd %BUILD_DIR%

cmake .. -G "Visual Studio 18 2026" ^
    -DCMAKE_TOOLCHAIN_FILE="build\%BUILD_TYPE%\generators\conan_toolchain.cmake" ^
    -DCMAKE_CXX_STANDARD=23 ^
    -DCMAKE_CXX_STANDARD_REQUIRED=ON

if %errorlevel% neq 0 (
    echo [ERROR] CMake config failed
    pause
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] %BUILD_TYPE% sln file generated
pause