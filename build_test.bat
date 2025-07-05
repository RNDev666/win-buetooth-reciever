@echo off
echo Building Bluetooth A2DP Sink Test Application...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake (skip vcpkg for now to test basic compilation)
echo Configuring with CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build both projects
echo Building projects...
cmake --build . --config Release

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build successful!
    echo ========================================
    echo Main Application: build\bin\Release\BluetoothAudioReceiver.exe
    echo Test Application: build\bin\Release\TestA2DPSink.exe
    echo.
    echo To test A2DP Sink functionality:
    echo   cd build\bin\Release
    echo   TestA2DPSink.exe
    echo.
    echo To run the full GUI application:
    echo   cd build\bin\Release
    echo   BluetoothAudioReceiver.exe
    echo.
) else (
    echo.
    echo ========================================
    echo Build failed!
    echo ========================================
    echo Check the output above for errors.
    echo.
    pause
    exit /b 1
)

cd ..
pause