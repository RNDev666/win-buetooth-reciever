@echo off
echo Creating deployment package for Bluetooth Audio Receiver...

REM Create deployment directory
if exist deployment rmdir /s /q deployment
mkdir deployment
mkdir deployment\bin
mkdir deployment\logs

REM Copy executable and dependencies
copy build\bin\Release\BluetoothAudioReceiver.exe deployment\bin\
copy build\bin\Release\*.dll deployment\bin\

REM Copy documentation
copy README.md deployment\
copy IMPLEMENTATION_STATUS.md deployment\

REM Create launch script
echo @echo off > deployment\BluetoothAudioReceiver.bat
echo cd /d "%%~dp0bin" >> deployment\BluetoothAudioReceiver.bat
echo start BluetoothAudioReceiver.exe >> deployment\BluetoothAudioReceiver.bat
echo echo Bluetooth Audio Receiver launched! >> deployment\BluetoothAudioReceiver.bat
echo pause >> deployment\BluetoothAudioReceiver.bat

echo.
echo ========================================
echo Deployment package created successfully!
echo ========================================
echo.
echo Package location: deployment\
echo.
echo To distribute:
echo   1. Copy the 'deployment' folder to target system
echo   2. Run 'BluetoothAudioReceiver.bat' to launch
echo.
echo The application includes:
echo   - Executable and all dependencies
echo   - Documentation
echo   - Launch script
echo.
pause 
