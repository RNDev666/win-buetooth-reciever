#pragma once
#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Audio.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <shellapi.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace Platform {

struct BluetoothDeviceInfo {
    std::string id;
    std::string name;
    std::string containerId;
    bool isPresent = false;
    bool isPaired = false;
};

struct AudioDeviceInfo {
    std::string id;
    std::string name;
    std::string description;
    bool isDefault = false;
    bool isEnabled = true;
    float volume = 1.0f;
    bool isMuted = false;
};

class WindowsAPI {
public:
    static bool Initialize();
    static void Cleanup();
    
    // Bluetooth operations
    static std::vector<BluetoothDeviceInfo> EnumerateBluetoothDevices();
    static bool EnableA2DPSink(const std::string& deviceId);
    static bool ConnectBluetoothAudio(const std::string& deviceId);
    static bool DisconnectBluetoothAudio(const std::string& deviceId);
    static bool IsBluetoothDeviceConnected(const std::string& deviceId);
    
    // Audio operations
    static std::vector<AudioDeviceInfo> EnumerateAudioDevices();
    static bool SetDefaultAudioDevice(const std::string& deviceId);
    static bool SetAudioDeviceVolume(const std::string& deviceId, float volume);
    static bool SetAudioDeviceMute(const std::string& deviceId, bool muted);
    static float GetAudioDeviceVolume(const std::string& deviceId);
    static bool IsAudioDeviceMuted(const std::string& deviceId);
    
    // System tray operations
    static bool CreateSystemTrayIcon(HWND hwnd, UINT callbackMsg, HICON icon = nullptr);
    static void DestroySystemTrayIcon();
    static bool UpdateSystemTrayIcon(const std::string& tooltip);
    static bool ShowSystemTrayBalloon(const std::string& title, const std::string& message);
    static void RemoveSystemTrayIcon();
    
    // Notification operations
    static void ShowNotification(const wchar_t* title, const wchar_t* message);
    
    // Utility functions
    static void LogHResult(const std::string& operation, HRESULT hr);
    static std::string GetLastErrorString();
    
private:
    // COM and device interfaces
    static winrt::com_ptr<IMMDeviceEnumerator> s_audioEnumerator;
    static bool s_initialized;
    
    // System tray data
    static NOTIFYICONDATA s_notifyIconData;
    static bool s_trayIconCreated;
    
    // Helper methods
    static std::string WideStringToUtf8(const std::wstring& wstr);
    static std::wstring Utf8ToWideString(const std::string& str);
    static HICON LoadDefaultIcon();
    static std::string GetHResultString(HRESULT hr);
};

} // namespace Platform 
