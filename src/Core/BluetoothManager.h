#pragma once
#include "../Models/BluetoothDevice.h"
#include "../Utils/Threading.h"
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace BluetoothAudio {

class BluetoothManager {
public:
    using DeviceCallback = std::function<void(const Models::BluetoothDevice&)>;
    using StatusCallback = std::function<void(const std::string&)>;
    
    BluetoothManager();
    ~BluetoothManager();
    
    bool Initialize();
    void Shutdown();
    
    // Device management
    void RefreshDevices();
    const std::vector<Models::BluetoothDevice>& GetDevices() const;
    Models::BluetoothDevice* GetDevice(const std::string& deviceId);
    
    // Connection management
    bool EnableDevice(const std::string& deviceId);
    bool ConnectDevice(const std::string& deviceId);
    bool DisconnectDevice(const std::string& deviceId);
    bool DisconnectAllDevices();
    
    // Status and callbacks
    void SetDeviceAddedCallback(DeviceCallback callback);
    void SetDeviceRemovedCallback(DeviceCallback callback);
    void SetDeviceUpdatedCallback(DeviceCallback callback);
    void SetStatusCallback(StatusCallback callback);
    
    // Connection state
    bool IsDeviceConnected(const std::string& deviceId) const;
    bool HasConnectedDevices() const;
    std::string GetConnectionStatus() const;
    
    // Statistics
    size_t GetDeviceCount() const;
    size_t GetConnectedDeviceCount() const;
    
private:
    void DeviceWatcherThread();
    void UpdateDeviceStatus(const std::string& deviceId, Models::ConnectionState state);
    void AddDevice(const Models::BluetoothDevice& device);
    void RemoveDevice(const std::string& deviceId);
    void UpdateDevice(const Models::BluetoothDevice& device);
    void NotifyStatusChange(const std::string& status);
    
    std::vector<Models::BluetoothDevice> m_devices;
    std::unordered_map<std::string, size_t> m_deviceIndexMap;
    mutable std::mutex m_devicesMutex;
    
    DeviceCallback m_deviceAddedCallback;
    DeviceCallback m_deviceRemovedCallback;
    DeviceCallback m_deviceUpdatedCallback;
    StatusCallback m_statusCallback;
    
    std::unique_ptr<Utils::WorkerThread> m_workerThread;
    std::unique_ptr<Utils::Timer> m_refreshTimer;
    
    std::atomic<bool> m_initialized{false};
    std::string m_currentStatus;
    mutable std::mutex m_statusMutex;
};

} // namespace BluetoothAudio 
