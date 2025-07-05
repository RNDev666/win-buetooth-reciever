@echo off
echo Building Bluetooth Audio Receiver...

REM Check if vcpkg is available
if not defined VCPKG_ROOT (
    echo Error: VCPKG_ROOT environment variable is not set.
    echo Please install vcpkg and set VCPKG_ROOT to the installation directory.
    echo Example: set VCPKG_ROOT=C:\vcpkg
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build successful!
    echo ========================================
    echo Executable: build\bin\Release\BluetoothAudioReceiver.exe
    echo.
    echo To run the application:
    echo   cd build\bin\Release
    echo   BluetoothAudioReceiver.exe
    echo.
) else (
    echo.
    echo ========================================
    echo Build failed!
    echo ========================================
    pause
    exit /b 1
)

cd ..
pause 
