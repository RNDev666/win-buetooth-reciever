#pragma once
#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Audio.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.Rfcomm.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Media.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <shellapi.h>
#include <comdef.h>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <mutex>

namespace Platform {

// Forward declarations
struct BluetoothDeviceInfo;
struct AudioDeviceInfo;

// Audio codec types
enum class AudioCodec {
    SBC,
    AAC,
    aptX,
    LDAC
};

// Audio stream callback
using AudioStreamCallback = std::function<void(const std::string& deviceId, const float* audioData, size_t sampleCount)>;

// Bluetooth connection callback
using BluetoothConnectionCallback = std::function<void(const std::string& deviceId, const std::string& deviceName, bool connected)>;

// Bluetooth device info structure
struct BluetoothDeviceInfo {
    std::string id;
    std::string name;
    std::string address;
    bool isPresent = false;
    bool isPaired = false;
    bool isConnected = false;
    bool isAudioCapable = false;
    int signalStrength = 0;
};

// Audio device info structure
struct AudioDeviceInfo {
    std::string id;
    std::string name;
    std::string description;
    bool isDefault = false;
    bool isEnabled = false;
    bool isMuted = false;
    float volume = 0.0f;
};

// A2DP Audio Stream Info
struct A2DPStreamInfo {
    AudioCodec codec = AudioCodec::SBC;
    uint32_t sampleRate = 44100;
    uint32_t channels = 2;
    uint32_t bitRate = 328;
    uint32_t frameSize = 4;
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
    
    // Bluetooth A2DP Sink Operations
    static bool StartBluetoothA2DPSink(const std::string& deviceName);
    static bool StopBluetoothA2DPSink();
    static bool IsA2DPSinkRunning();
    static bool SetDeviceDiscoverable(bool discoverable);
    static bool SetDevicePairable(bool pairable);
    
    // Connection Management
    static bool AcceptIncomingConnection(const std::string& deviceId);
    static bool RejectIncomingConnection(const std::string& deviceId);
    static bool DisconnectBluetoothDevice(const std::string& deviceId);
    static std::vector<std::string> GetConnectedAudioSources();
    
    // Audio Stream Management
    static bool StartAudioStreamReception(const std::string& deviceId, const A2DPStreamInfo& streamInfo);
    static bool StopAudioStreamReception(const std::string& deviceId);
    static bool IsAudioStreamActive(const std::string& deviceId);
    
    // Audio Codec Support
    static bool DecodeSBCAudio(const uint8_t* encodedData, size_t encodedSize, 
                              float* decodedData, size_t* decodedSize, const A2DPStreamInfo& streamInfo);
    static bool DecodeAACAudio(const uint8_t* encodedData, size_t encodedSize, 
                              float* decodedData, size_t* decodedSize, const A2DPStreamInfo& streamInfo);
    
    // Callback Management
    static void SetAudioStreamCallback(AudioStreamCallback callback);
    static void SetBluetoothConnectionCallback(BluetoothConnectionCallback callback);
    
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
    
    // Bluetooth Service Management
    static bool RegisterA2DPSinkService();
    static bool UnregisterA2DPSinkService();
    static void HandleIncomingA2DPConnection(const std::string& deviceId, const std::string& deviceName);
    static void HandleA2DPAudioData(const std::string& deviceId, const uint8_t* audioData, size_t dataSize);
    
    // Audio Processing
    static void ProcessIncomingAudioData(const std::string& deviceId, const uint8_t* data, size_t size);
    static bool InitializeAudioDecoder(AudioCodec codec);
    static void CleanupAudioDecoder(AudioCodec codec);
    
    // Bluetooth state
    static AudioStreamCallback s_audioStreamCallback;
    static BluetoothConnectionCallback s_connectionCallback;
    static std::string s_deviceName;
    static bool s_isDiscoverable;
    static bool s_isPairable;
    
    // Audio decoder state
    static void* s_sbcDecoder;
    static void* s_aacDecoder;
    
    // Connection tracking
    static std::vector<std::string> s_connectedDevices;
    static std::mutex s_devicesMutex;
};

} // namespace Platform 
