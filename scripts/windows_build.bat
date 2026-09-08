@echo off
setlocal enabledelayedexpansion

cd ..
set PROJECT_ROOT=%cd%

set BUILD_TYPE=
set BUILD_SUBDIR=
set DO_CLEAN=0

:parse_args
if "%~1"=="" goto after_args
if /i "%~1"=="debug"   (set BUILD_TYPE=Debug&   set BUILD_SUBDIR=debug)
if /i "%~1"=="release" (set BUILD_TYPE=Release& set BUILD_SUBDIR=release)
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
    echo           windows_build.bat debug clean
    pause
    exit /b 1
)

echo [INFO] Build type: %BUILD_TYPE%

conan export scripts\imguizmo

if %DO_CLEAN%==1 (
    if exist build (
        echo [INFO] Clean requested: removing build\
        rmdir /s /q build
    )
)

set TOOLCHAIN=%PROJECT_ROOT%\build\build\%BUILD_TYPE%\generators\conan_toolchain.cmake
set VCVARS=%PROJECT_ROOT%\build\build\%BUILD_TYPE%\generators\conanbuild.bat

if not exist "%TOOLCHAIN%" (
    echo [INFO] Conan install: %BUILD_TYPE%...
    conan install . --output-folder=build --build=missing ^
        -s build_type=%BUILD_TYPE% ^
        -s compiler.cppstd=23 ^
        -c tools.cmake.cmaketoolchain:generator=Ninja
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

if not exist "%VCVARS%" (
    echo [ERROR] conanbuild.bat not found at: %VCVARS%
    echo         Cannot set up MSVC compiler environment.
    pause
    exit /b 1
)

echo [INFO] Setting up MSVC compiler environment...
call "%VCVARS%"
if !errorlevel! neq 0 (
    echo [ERROR] Failed to set up MSVC environment
    pause
    exit /b !errorlevel!
)

where cl.exe >nul 2>nul
if !errorlevel! neq 0 (
    echo [ERROR] cl.exe still not found in PATH after conanbuild.bat
    pause
    exit /b 1
)

mkdir "build\%BUILD_SUBDIR%" 2>nul
cd "build\%BUILD_SUBDIR%"

cmake "%PROJECT_ROOT%" -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
    -DCMAKE_CXX_STANDARD=23 ^
    -DCMAKE_CXX_STANDARD_REQUIRED=ON ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if %errorlevel% neq 0 (
    echo [ERROR] CMake config failed
    pause
    exit /b %errorlevel%
)

echo [INFO] Building with Ninja...
cmake --build . -j %NUMBER_OF_PROCESSORS%

if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    pause
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] %BUILD_TYPE% build complete.
pause