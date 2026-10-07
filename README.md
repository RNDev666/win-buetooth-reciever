# Bluetooth Audio Receiver

A modern Windows application that turns your PC into a Bluetooth audio receiver (A2DP Sink) using Windows 10 2004+ native capabilities. Built with modern C++20 and Dear ImGui for a responsive, lightweight interface.

## Features

- **Bluetooth A2DP Sink**: Receive audio from any Bluetooth device
- **Modern UI**: Clean, responsive interface using Dear ImGui
- **System Tray Integration**: Minimize to tray with quick access menu
- **Audio Device Management**: Select output devices and control volume
- **Auto-connect**: Automatically connect to previously used devices
- **Logging**: Comprehensive logging for troubleshooting
- **Low Resource Usage**: Minimal CPU and memory footprint

## Requirements

- **Windows 10 version 2004 (20H1) or later**
- **Bluetooth hardware** with A2DP Sink support
- **Visual Studio 2019/2022** or compatible C++20 compiler
- **vcpkg** package manager
- **CMake 3.25+**

## Quick Start

### 1. Install Dependencies

First, install and set up vcpkg:

```batch
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

Set the VCPKG_ROOT environment variable:
```batch
set VCPKG_ROOT=C:\path\to\vcpkg
```

### 2. Build the Application

```batch
git clone <repository-url>
cd bluetooth-audio-receiver
build.bat
```

### 3. Run the Application

```batch
cd build\bin\Release
BluetoothAudioReceiver.exe
```

## Manual Build Instructions

If you prefer to build manually:

```batch
# Install dependencies
vcpkg install imgui[dx11-binding,win32-binding]:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install spdlog:x64-windows
vcpkg install fmt:x64-windows

# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Usage

### First Time Setup

1. **Pair your Bluetooth device** with Windows through Settings > Devices > Bluetooth
2. **Launch the application** - it will appear in the system tray
3. **Click the tray icon** or open the main window
4. **Select your device** from the list
5. **Click "Enable"** to prepare the device for audio reception
6. **Click "Connect"** to start receiving audio

### System Tray

The application runs in the system tray for easy access:
- **Single click**: Show/hide main window
- **Right click**: Quick access menu with:
  - Connected devices
  - Settings
  - Exit

### Audio Routing

- Audio received via Bluetooth is played through your selected output device
- You can change the output device in the Audio Settings section
- Volume control is available for compatible devices

## Configuration

Configuration is stored in `%LOCALAPPDATA%\BluetoothAudioReceiver\config.json`:

```json
{
  "minimizeToTray": true,
  "autoConnectLast": true,
  "showNotifications": true,
  "preferredOutputDevice": "",
  "windowWidth": 800,
  "windowHeight": 600
}
```

## Troubleshooting

### Common Issues

**"No Bluetooth devices found"**
- Ensure devices are paired in Windows Settings first
- Check that your Bluetooth adapter supports A2DP Sink
- Restart the Bluetooth service: `services.msc` → Bluetooth Support Service

**"Failed to enable A2DP Sink"**
- Verify Windows 10 version 2004+ with: `winver`
- Check Windows Update for latest drivers
- Some Bluetooth adapters may not support A2DP Sink

**"Connection failed"**
- Ensure the device isn't connected to another source
- Try forgetting and re-pairing the device
- Check Windows Event Viewer for Bluetooth errors

### Logs

Detailed logs are available in the `logs/` directory:
- `bluetooth_audio_receiver.log` - Main application log
- Log level can be adjusted in settings

### Compatibility

**Tested Bluetooth Adapters:**
- Intel AX200/AX201 series
- Qualcomm/Atheros chipsets
- Realtek RTL8822CE

**Known Issues:**
- Some older Bluetooth adapters may not support A2DP Sink
- USB Bluetooth dongles may have limited A2DP Sink support

## Development

### Project Structure

```
src/
├── Core/           # Core business logic
├── Models/         # Data models
├── Platform/       # Windows API wrappers
├── UI/            # ImGui interface
├── Utils/         # Utility classes
└── main.cpp       # Entry point
```

### Key Technologies

- **C++20**: Modern C++ features and STL
- **Dear ImGui**: Immediate mode GUI framework
- **WinRT**: Windows Runtime APIs for Bluetooth
- **DirectX 11**: Graphics rendering backend
- **spdlog**: Fast logging library
- **nlohmann/json**: JSON configuration

### Building from Source

Requirements:
- Visual Studio 2019/2022 with C++20 support
- Windows SDK 10.0.19041.0 or later
- vcpkg package manager

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Microsoft for Windows A2DP Sink APIs
- Dear ImGui team for the excellent UI framework
- AudioPlaybackConnector project for inspiration
- Windows community for testing and feedback

## Support

For issues and questions:
1. Check the [Issues](../../issues) page
2. Review the troubleshooting section
3. Enable debug logging for detailed information
4. Create a new issue with logs and system information 

If you find this useful, you can also support development on Ko-fi:

[![Support me on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/rndev666)
