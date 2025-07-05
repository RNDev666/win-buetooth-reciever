#include "BluetoothManager.h"
#include "../Platform/WindowsAPI.h"
#include "../Utils/Logger.h"
#include <algorithm>

namespace BluetoothAudio {

BluetoothManager::BluetoothManager() 
    : m_workerThread(std::make_unique<Utils::WorkerThread>())
    , m_refreshTimer(std::make_unique<Utils::Timer>()) {
}

BluetoothManager::~BluetoothManager() {
    Shutdown();
}

bool BluetoothManager::Initialize() {
    if (m_initialized) return true;
    
    Utils::Logger::Info("Initializing Bluetooth Manager...");
    
    // Start worker thread for background operations
    m_workerThread->Start();
    
    // Initial device refresh
    RefreshDevices();
    
    // Start periodic refresh timer (every 5 seconds)
    m_refreshTimer->Start(std::chrono::milliseconds(5000), [this]() {
        RefreshDevices();
    });
    
    m_initialized = true;
    NotifyStatusChange("Bluetooth Manager initialized");
    
    Utils::Logger::Info("Bluetooth Manager initialized successfully");
    return true;
}

void BluetoothManager::Shutdown() {
    if (!m_initialized) return;
    
    Utils::Logger::Info("Shutting down Bluetooth Manager...");
    
    // Disconnect all devices
    DisconnectAllDevices();
    
    // Stop timer and worker thread
    m_refreshTimer->Stop();
    m_workerThread->Stop();
    
    // Clear devices
    {
        std::lock_guard<std::mutex> lock(m_devicesMutex);
        m_devices.clear();
        m_deviceIndexMap.clear();
    }
    
    m_initialized = false;
    NotifyStatusChange("Bluetooth Manager shut down");
    
    Utils::Logger::Info("Bluetooth Manager shut down complete");
}

void BluetoothManager::RefreshDevices() {
    if (!m_initialized) return;
    
    m_workerThread->PostTask([this]() {
        try {
            auto deviceInfos = Platform::WindowsAPI::EnumerateBluetoothDevices();
            
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            
            // Track which devices we've seen in this refresh
            std::vector<std::string> seenDeviceIds;
            
            for (const auto& info : deviceInfos) {
                seenDeviceIds.push_back(info.id);
                
                auto it = m_deviceIndexMap.find(info.id);
                if (it != m_deviceIndexMap.end()) {
                    // Update existing device
                    auto& device = m_devices[it->second];
                    bool wasConnected = device.isConnected;
                    
                    device.name = info.name;
                    device.containerId = info.containerId;
                    device.lastSeen = std::chrono::system_clock::now();
                    device.isConnected = Platform::WindowsAPI::IsBluetoothDeviceConnected(info.id);
                    
                    // Check for connection state changes
                    if (device.isConnected != wasConnected) {
                        device.state = device.isConnected ? 
                            Models::ConnectionState::Connected : 
                            Models::ConnectionState::Enabled;
                        
                        Utils::Logger::Info("Device connection state changed: " + device.name + " - " + (device.isConnected ? "Connected" : "Disconnected"));
                        
                        if (m_deviceUpdatedCallback) {
                            m_deviceUpdatedCallback(device);
                        }
                    }
                } else {
                    // Add new device
                    Models::BluetoothDevice device;
                    device.id = info.id;
                    device.name = info.name;
                    device.containerId = info.containerId;
                    device.state = Models::ConnectionState::Disconnected;
                    device.isEnabled = false;
                    device.isConnected = Platform::WindowsAPI::IsBluetoothDeviceConnected(info.id);
                    device.lastSeen = std::chrono::system_clock::now();
                    
                    if (device.isConnected) {
                        device.state = Models::ConnectionState::Connected;
                        device.isEnabled = true;
                    }
                    
                    AddDevice(device);
                    
                    Utils::Logger::Info("New Bluetooth device discovered: " + device.name + " (" + device.id + ")");
                }
            }
            
            // Remove devices that were not seen in this refresh
            auto it = m_devices.begin();
            while (it != m_devices.end()) {
                if (std::find(seenDeviceIds.begin(), seenDeviceIds.end(), it->id) == seenDeviceIds.end()) {
                    Utils::Logger::Info("Bluetooth device removed: " + it->name + " (" + it->id + ")");
                    
                    if (m_deviceRemovedCallback) {
                        m_deviceRemovedCallback(*it);
                    }
                    
                    m_deviceIndexMap.erase(it->id);
                    it = m_devices.erase(it);
                    
                    // Update indices
                    for (size_t i = 0; i < m_devices.size(); ++i) {
                        m_deviceIndexMap[m_devices[i].id] = i;
                    }
                } else {
                    ++it;
                }
            }
            
        } catch (const std::exception& e) {
            Utils::Logger::Error("Error during device refresh: " + std::string(e.what()));
        }
    });
}

const std::vector<Models::BluetoothDevice>& BluetoothManager::GetDevices() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_devices;
}

Models::BluetoothDevice* BluetoothManager::GetDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_deviceIndexMap.find(deviceId);
    return it != m_deviceIndexMap.end() ? &m_devices[it->second] : nullptr;
}

bool BluetoothManager::EnableDevice(const std::string& deviceId) {
    Utils::Logger::Info("Enabling device: " + deviceId);
    
    UpdateDeviceStatus(deviceId, Models::ConnectionState::Enabling);
    
    bool success = Platform::WindowsAPI::EnableA2DPSink(deviceId);
    
    if (success) {
        UpdateDeviceStatus(deviceId, Models::ConnectionState::Enabled);
        
        // Update device state
        if (auto device = GetDevice(deviceId)) {
            device->isEnabled = true;
        }
        
        NotifyStatusChange("Device enabled: " + deviceId);
        Utils::Logger::Info("Successfully enabled device: " + deviceId);
    } else {
        UpdateDeviceStatus(deviceId, Models::ConnectionState::Error);
        NotifyStatusChange("Failed to enable device: " + deviceId);
        Utils::Logger::Error("Failed to enable device: " + deviceId);
    }
    
    return success;
}

bool BluetoothManager::ConnectDevice(const std::string& deviceId) {
    Utils::Logger::Info("Connecting to device: " + deviceId);
    
    UpdateDeviceStatus(deviceId, Models::ConnectionState::Connecting);
    
    bool success = Platform::WindowsAPI::ConnectBluetoothAudio(deviceId);
    
    if (success) {
        UpdateDeviceStatus(deviceId, Models::ConnectionState::Connected);
        
        // Update device state
        if (auto device = GetDevice(deviceId)) {
            device->isConnected = true;
            device->isEnabled = true;
        }
        
        NotifyStatusChange("Connected to: " + deviceId);
        Utils::Logger::Info("Successfully connected to device: " + deviceId);
    } else {
        UpdateDeviceStatus(deviceId, Models::ConnectionState::Error);
        NotifyStatusChange("Failed to connect to: " + deviceId);
        Utils::Logger::Error("Failed to connect to device: " + deviceId);
    }
    
    return success;
}

bool BluetoothManager::DisconnectDevice(const std::string& deviceId) {
    Utils::Logger::Info("Disconnecting from device: " + deviceId);
    
    bool success = Platform::WindowsAPI::DisconnectBluetoothAudio(deviceId);
    
    if (success) {
        UpdateDeviceStatus(deviceId, Models::ConnectionState::Enabled);
        
        // Update device state
        if (auto device = GetDevice(deviceId)) {
            device->isConnected = false;
        }
        
        NotifyStatusChange("Disconnected from: " + deviceId);
        Utils::Logger::Info("Successfully disconnected from device: " + deviceId);
    } else {
        NotifyStatusChange("Failed to disconnect from: " + deviceId);
        Utils::Logger::Error("Failed to disconnect from device: " + deviceId);
    }
    
    return success;
}

bool BluetoothManager::DisconnectAllDevices() {
    Utils::Logger::Info("Disconnecting all devices...");
    
    bool allSuccess = true;
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    
    for (auto& device : m_devices) {
        if (device.isConnected) {
            if (!DisconnectDevice(device.id)) {
                allSuccess = false;
            }
        }
    }
    
    return allSuccess;
}

void BluetoothManager::SetDeviceAddedCallback(DeviceCallback callback) {
    m_deviceAddedCallback = callback;
}

void BluetoothManager::SetDeviceRemovedCallback(DeviceCallback callback) {
    m_deviceRemovedCallback = callback;
}

void BluetoothManager::SetDeviceUpdatedCallback(DeviceCallback callback) {
    m_deviceUpdatedCallback = callback;
}

void BluetoothManager::SetStatusCallback(StatusCallback callback) {
    m_statusCallback = callback;
}

bool BluetoothManager::IsDeviceConnected(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_deviceIndexMap.find(deviceId);
    return it != m_deviceIndexMap.end() ? m_devices[it->second].isConnected : false;
}

bool BluetoothManager::HasConnectedDevices() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return std::any_of(m_devices.begin(), m_devices.end(), 
        [](const Models::BluetoothDevice& device) { return device.isConnected; });
}

std::string BluetoothManager::GetConnectionStatus() const {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_currentStatus;
}

size_t BluetoothManager::GetDeviceCount() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_devices.size();
}

size_t BluetoothManager::GetConnectedDeviceCount() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return std::count_if(m_devices.begin(), m_devices.end(),
        [](const Models::BluetoothDevice& device) { return device.isConnected; });
}

// Private methods
void BluetoothManager::UpdateDeviceStatus(const std::string& deviceId, Models::ConnectionState state) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_deviceIndexMap.find(deviceId);
    if (it != m_deviceIndexMap.end()) {
        auto& device = m_devices[it->second];
        device.state = state;
        device.errorMessage.clear();
        
        if (state == Models::ConnectionState::Error) {
            device.errorMessage = "Connection failed";
        }
        
        if (m_deviceUpdatedCallback) {
            m_deviceUpdatedCallback(device);
        }
    }
}

void BluetoothManager::AddDevice(const Models::BluetoothDevice& device) {
    // Assumes m_devicesMutex is already locked
    m_deviceIndexMap[device.id] = m_devices.size();
    m_devices.push_back(device);
    
    if (m_deviceAddedCallback) {
        m_deviceAddedCallback(device);
    }
}

void BluetoothManager::RemoveDevice(const std::string& deviceId) {
    // Assumes m_devicesMutex is already locked
    auto it = m_deviceIndexMap.find(deviceId);
    if (it != m_deviceIndexMap.end()) {
        size_t index = it->second;
        
        if (m_deviceRemovedCallback) {
            m_deviceRemovedCallback(m_devices[index]);
        }
        
        m_devices.erase(m_devices.begin() + index);
        m_deviceIndexMap.erase(it);
        
        // Update indices for remaining devices
        for (size_t i = index; i < m_devices.size(); ++i) {
            m_deviceIndexMap[m_devices[i].id] = i;
        }
    }
}

void BluetoothManager::UpdateDevice(const Models::BluetoothDevice& device) {
    // Assumes m_devicesMutex is already locked
    auto it = m_deviceIndexMap.find(device.id);
    if (it != m_deviceIndexMap.end()) {
        m_devices[it->second] = device;
        
        if (m_deviceUpdatedCallback) {
            m_deviceUpdatedCallback(device);
        }
    }
}

void BluetoothManager::NotifyStatusChange(const std::string& status) {
    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_currentStatus = status;
    }
    
    if (m_statusCallback) {
        m_statusCallback(status);
    }
}

} // namespace BluetoothAudio 
