@echo off
echo ========================================
echo Bluetooth Audio Receiver Setup
echo ========================================
echo.

REM Check if vcpkg is available
if not defined VCPKG_ROOT (
    echo Error: VCPKG_ROOT environment variable is not set.
    echo.
    echo Please install vcpkg first:
    echo 1. Clone vcpkg: git clone https://github.com/Microsoft/vcpkg.git
    echo 2. Bootstrap: .\vcpkg\bootstrap-vcpkg.bat
    echo 3. Integrate: .\vcpkg\vcpkg integrate install
    echo 4. Set environment variable: set VCPKG_ROOT=C:\path\to\vcpkg
    echo.
    pause
    exit /b 1
)

echo Found vcpkg at: %VCPKG_ROOT%
echo.

REM Check if vcpkg.exe exists
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error: vcpkg.exe not found at %VCPKG_ROOT%
    echo Please ensure vcpkg is properly installed and bootstrapped.
    pause
    exit /b 1
)

echo Installing dependencies...
echo.

REM Install imgui with DirectX 11 and Win32 bindings
echo Installing Dear ImGui...
"%VCPKG_ROOT%\vcpkg.exe" install imgui[dx11-binding,win32-binding]:x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install imgui
    pause
    exit /b 1
)

REM Install nlohmann-json
echo Installing nlohmann-json...
"%VCPKG_ROOT%\vcpkg.exe" install nlohmann-json:x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install nlohmann-json
    pause
    exit /b 1
)

REM Install spdlog
echo Installing spdlog...
"%VCPKG_ROOT%\vcpkg.exe" install spdlog:x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install spdlog
    pause
    exit /b 1
)

REM Install fmt
echo Installing fmt...
"%VCPKG_ROOT%\vcpkg.exe" install fmt:x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo Failed to install fmt
    pause
    exit /b 1
)

echo.
echo ========================================
echo Setup completed successfully!
echo ========================================
echo.
echo All dependencies have been installed.
echo You can now build the project with: build.bat
echo.
pause 
