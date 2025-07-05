#include "AudioManager.h"
#include "../Platform/WindowsAPI.h"
#include "../Utils/Logger.h"
#include <algorithm>

namespace BluetoothAudio {

AudioManager::AudioManager() 
    : m_workerThread(std::make_unique<Utils::WorkerThread>())
    , m_refreshTimer(std::make_unique<Utils::Timer>()) {
}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize() {
    if (m_initialized) return true;
    
    Utils::Logger::Info("Initializing Audio Manager...");
    
    // Start worker thread for background operations
    m_workerThread->Start();
    
    // Initial device refresh
    RefreshDevices();
    
    // Start periodic refresh timer (every 3 seconds)
    m_refreshTimer->Start(std::chrono::milliseconds(3000), [this]() {
        RefreshDevices();
    });
    
    m_initialized = true;
    NotifyStatusChange("Audio Manager initialized");
    
    Utils::Logger::Info("Audio Manager initialized successfully");
    return true;
}

void AudioManager::Shutdown() {
    if (!m_initialized) return;
    
    Utils::Logger::Info("Shutting down Audio Manager...");
    
    // Stop timer and worker thread
    m_refreshTimer->Stop();
    m_workerThread->Stop();
    
    // Clear devices
    {
        std::lock_guard<std::mutex> lock(m_devicesMutex);
        m_devices.clear();
        m_deviceIndexMap.clear();
        m_defaultDeviceId.clear();
    }
    
    m_initialized = false;
    NotifyStatusChange("Audio Manager shut down");
    
    Utils::Logger::Info("Audio Manager shut down complete");
}

void AudioManager::RefreshDevices() {
    if (!m_initialized) return;
    
    m_workerThread->PostTask([this]() {
        try {
            auto deviceInfos = Platform::WindowsAPI::EnumerateAudioDevices();
            
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            
            // Track which devices we've seen in this refresh
            std::vector<std::string> seenDeviceIds;
            std::string currentDefaultId;
            
            for (const auto& info : deviceInfos) {
                seenDeviceIds.push_back(info.id);
                
                if (info.isDefault) {
                    currentDefaultId = info.id;
                }
                
                auto it = m_deviceIndexMap.find(info.id);
                if (it != m_deviceIndexMap.end()) {
                    // Update existing device
                    auto& device = m_devices[it->second];
                    bool volumeChanged = (device.volume != info.volume);
                    bool muteChanged = (device.isMuted != info.isMuted);
                    bool defaultChanged = (device.isDefault != info.isDefault);
                    
                    device.name = info.name;
                    device.description = info.description;
                    device.isDefault = info.isDefault;
                    device.isEnabled = info.isEnabled;
                    device.volume = info.volume;
                    device.isMuted = info.isMuted;
                    
                    // Notify if significant changes occurred
                    if (volumeChanged || muteChanged || defaultChanged) {
                        if (m_deviceUpdatedCallback) {
                            m_deviceUpdatedCallback(device);
                        }
                    }
                } else {
                    // Add new device
                    Models::AudioDevice device;
                    device.id = info.id;
                    device.name = info.name;
                    device.description = info.description;
                    device.isDefault = info.isDefault;
                    device.isEnabled = info.isEnabled;
                    device.volume = info.volume;
                    device.isMuted = info.isMuted;
                    
                    AddDevice(device);
                    
                    Utils::Logger::Info("New audio device discovered: " + device.name + " (" + device.id + ")");
                }
            }
            
            // Update default device ID
            if (currentDefaultId != m_defaultDeviceId) {
                m_defaultDeviceId = currentDefaultId;
                Utils::Logger::Info("Default audio device changed to: " + m_defaultDeviceId);
            }
            
            // Remove devices that were not seen in this refresh
            auto it = m_devices.begin();
            while (it != m_devices.end()) {
                if (std::find(seenDeviceIds.begin(), seenDeviceIds.end(), it->id) == seenDeviceIds.end()) {
                    Utils::Logger::Info("Audio device removed: " + it->name + " (" + it->id + ")");
                    
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
            Utils::Logger::Error("Error during audio device refresh: " + std::string(e.what()));
        }
    });
}

const std::vector<Models::AudioDevice>& AudioManager::GetDevices() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_devices;
}

Models::AudioDevice* AudioManager::GetDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_deviceIndexMap.find(deviceId);
    return it != m_deviceIndexMap.end() ? &m_devices[it->second] : nullptr;
}

Models::AudioDevice* AudioManager::GetDefaultDevice() {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    if (m_defaultDeviceId.empty()) return nullptr;
    
    auto it = m_deviceIndexMap.find(m_defaultDeviceId);
    return it != m_deviceIndexMap.end() ? &m_devices[it->second] : nullptr;
}

bool AudioManager::SetDefaultDevice(const std::string& deviceId) {
    Utils::Logger::Info("Setting default audio device: " + deviceId);
    
    bool success = Platform::WindowsAPI::SetDefaultAudioDevice(deviceId);
    
    if (success) {
        {
            std::lock_guard<std::mutex> lock(m_devicesMutex);
            
            // Update old default device
            if (!m_defaultDeviceId.empty()) {
                auto oldIt = m_deviceIndexMap.find(m_defaultDeviceId);
                if (oldIt != m_deviceIndexMap.end()) {
                    m_devices[oldIt->second].isDefault = false;
                }
            }
            
            // Update new default device
            auto newIt = m_deviceIndexMap.find(deviceId);
            if (newIt != m_deviceIndexMap.end()) {
                m_devices[newIt->second].isDefault = true;
            }
            
            m_defaultDeviceId = deviceId;
        }
        
        NotifyStatusChange("Default device changed to: " + deviceId);
        Utils::Logger::Info("Successfully set default audio device: " + deviceId);
    } else {
        NotifyStatusChange("Failed to set default device: " + deviceId);
        Utils::Logger::Error("Failed to set default audio device: " + deviceId);
    }
    
    return success;
}

bool AudioManager::SetDeviceVolume(const std::string& deviceId, float volume) {
    // Clamp volume to valid range
    volume = std::max(0.0f, std::min(1.0f, volume));
    
    bool success = Platform::WindowsAPI::SetAudioDeviceVolume(deviceId, volume);
    
    if (success) {
        // Update device state
        if (auto device = GetDevice(deviceId)) {
            device->volume = volume;
            
            if (m_deviceUpdatedCallback) {
                m_deviceUpdatedCallback(*device);
            }
        }
        
        Utils::Logger::Debug("Set device " + deviceId + " volume to: " + std::to_string(volume));
    } else {
        Utils::Logger::Error("Failed to set device " + deviceId + " volume to: " + std::to_string(volume));
    }
    
    return success;
}

bool AudioManager::SetDeviceMute(const std::string& deviceId, bool muted) {
    bool success = Platform::WindowsAPI::SetAudioDeviceMute(deviceId, muted);
    
    if (success) {
        // Update device state
        if (auto device = GetDevice(deviceId)) {
            device->isMuted = muted;
            
            if (m_deviceUpdatedCallback) {
                m_deviceUpdatedCallback(*device);
            }
        }
        
        Utils::Logger::Debug("Set device " + deviceId + " mute to: " + (muted ? "true" : "false"));
    } else {
        Utils::Logger::Error("Failed to set device " + deviceId + " mute to: " + (muted ? "true" : "false"));
    }
    
    return success;
}

bool AudioManager::MuteAllDevices() {
    Utils::Logger::Info("Muting all audio devices...");
    
    bool allSuccess = true;
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    
    for (auto& device : m_devices) {
        if (!device.isMuted) {
            if (!SetDeviceMute(device.id, true)) {
                allSuccess = false;
            }
        }
    }
    
    if (allSuccess) {
        NotifyStatusChange("All devices muted");
    } else {
        NotifyStatusChange("Some devices failed to mute");
    }
    
    return allSuccess;
}

bool AudioManager::UnmuteAllDevices() {
    Utils::Logger::Info("Unmuting all audio devices...");
    
    bool allSuccess = true;
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    
    for (auto& device : m_devices) {
        if (device.isMuted) {
            if (!SetDeviceMute(device.id, false)) {
                allSuccess = false;
            }
        }
    }
    
    if (allSuccess) {
        NotifyStatusChange("All devices unmuted");
    } else {
        NotifyStatusChange("Some devices failed to unmute");
    }
    
    return allSuccess;
}

std::string AudioManager::GetDefaultDeviceId() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_defaultDeviceId;
}

float AudioManager::GetDeviceVolume(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_deviceIndexMap.find(deviceId);
    return it != m_deviceIndexMap.end() ? m_devices[it->second].volume : 0.0f;
}

bool AudioManager::IsDeviceMuted(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    auto it = m_deviceIndexMap.find(deviceId);
    return it != m_deviceIndexMap.end() ? m_devices[it->second].isMuted : false;
}

size_t AudioManager::GetDeviceCount() const {
    std::lock_guard<std::mutex> lock(m_devicesMutex);
    return m_devices.size();
}

void AudioManager::SetDeviceAddedCallback(DeviceCallback callback) {
    m_deviceAddedCallback = callback;
}

void AudioManager::SetDeviceRemovedCallback(DeviceCallback callback) {
    m_deviceRemovedCallback = callback;
}

void AudioManager::SetDeviceUpdatedCallback(DeviceCallback callback) {
    m_deviceUpdatedCallback = callback;
}

void AudioManager::SetStatusCallback(StatusCallback callback) {
    m_statusCallback = callback;
}

std::string AudioManager::GetStatus() const {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_currentStatus;
}

// Private methods
void AudioManager::AddDevice(const Models::AudioDevice& device) {
    // Assumes m_devicesMutex is already locked
    m_deviceIndexMap[device.id] = m_devices.size();
    m_devices.push_back(device);
    
    if (m_deviceAddedCallback) {
        m_deviceAddedCallback(device);
    }
}

void AudioManager::RemoveDevice(const std::string& deviceId) {
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

void AudioManager::UpdateDevice(const Models::AudioDevice& device) {
    // Assumes m_devicesMutex is already locked
    auto it = m_deviceIndexMap.find(device.id);
    if (it != m_deviceIndexMap.end()) {
        m_devices[it->second] = device;
        
        if (m_deviceUpdatedCallback) {
            m_deviceUpdatedCallback(device);
        }
    }
}

void AudioManager::NotifyStatusChange(const std::string& status) {
    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_currentStatus = status;
    }
    
    if (m_statusCallback) {
        m_statusCallback(status);
    }
}

} // namespace BluetoothAudio 
