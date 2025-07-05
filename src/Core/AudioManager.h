#pragma once
#include "../Models/AudioDevice.h"
#include "../Utils/Threading.h"
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace BluetoothAudio {

class AudioManager {
public:
    using DeviceCallback = std::function<void(const Models::AudioDevice&)>;
    using StatusCallback = std::function<void(const std::string&)>;
    
    AudioManager();
    ~AudioManager();
    
    bool Initialize();
    void Shutdown();
    
    // Device management
    void RefreshDevices();
    const std::vector<Models::AudioDevice>& GetDevices() const;
    Models::AudioDevice* GetDevice(const std::string& deviceId);
    Models::AudioDevice* GetDefaultDevice();
    
    // Audio control
    bool SetDefaultDevice(const std::string& deviceId);
    bool SetDeviceVolume(const std::string& deviceId, float volume);
    bool SetDeviceMute(const std::string& deviceId, bool muted);
    bool MuteAllDevices();
    bool UnmuteAllDevices();
    
    // Device information
    std::string GetDefaultDeviceId() const;
    float GetDeviceVolume(const std::string& deviceId) const;
    bool IsDeviceMuted(const std::string& deviceId) const;
    size_t GetDeviceCount() const;
    
    // Callbacks
    void SetDeviceAddedCallback(DeviceCallback callback);
    void SetDeviceRemovedCallback(DeviceCallback callback);
    void SetDeviceUpdatedCallback(DeviceCallback callback);
    void SetStatusCallback(StatusCallback callback);
    
    // Status
    std::string GetStatus() const;
    bool IsInitialized() const { return m_initialized; }
    
private:
    void AddDevice(const Models::AudioDevice& device);
    void RemoveDevice(const std::string& deviceId);
    void UpdateDevice(const Models::AudioDevice& device);
    void NotifyStatusChange(const std::string& status);
    
    // Member variables
    std::atomic<bool> m_initialized{false};
    std::unique_ptr<Utils::WorkerThread> m_workerThread;
    std::unique_ptr<Utils::Timer> m_refreshTimer;
    
    mutable std::mutex m_devicesMutex;
    std::vector<Models::AudioDevice> m_devices;
    std::unordered_map<std::string, size_t> m_deviceIndexMap;
    
    mutable std::mutex m_statusMutex;
    std::string m_currentStatus;
    std::string m_defaultDeviceId;
    
    // Callbacks
    DeviceCallback m_deviceAddedCallback;
    DeviceCallback m_deviceRemovedCallback;
    DeviceCallback m_deviceUpdatedCallback;
    StatusCallback m_statusCallback;
};

} // namespace BluetoothAudio 
