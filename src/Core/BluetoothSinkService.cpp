#include "BluetoothSinkService.h"
#include "../Platform/WindowsAPI.h"
#include "../Utils/Logger.h"
#include "../Utils/StringHelpers.h"
#include <algorithm>
#include <chrono>

namespace BluetoothAudio {

BluetoothSinkService::BluetoothSinkService() 
    : m_serviceThread(std::make_unique<Utils::WorkerThread>())
    , m_monitorTimer(std::make_unique<Utils::Timer>()) {
    
    m_deviceName = "Bluetooth Audio Receiver";
    m_currentStatus = "Stopped";
    m_tempAudioBuffer.reserve(AUDIO_BUFFER_SIZE);
}

BluetoothSinkService::~BluetoothSinkService() {
    Shutdown();
}

bool BluetoothSinkService::Initialize() {
    if (m_initialized) return true;
    
    Utils::Logger::Info("Initializing Bluetooth Sink Service...");
    
    try {
        // Initialize A2DP sink
        if (!InitializeA2DPSink()) {
            Utils::Logger::Error("Failed to initialize A2DP sink");
            return false;
        }
        
        // Start service thread
        m_serviceThread->Start();
        
        // Start connection monitoring
        m_monitorTimer->Start(std::chrono::milliseconds(SERVICE_MONITOR_INTERVAL_MS), 
                             [this]() { MonitorConnections(); });
        
        m_initialized = true;
        NotifyStatusChange("Bluetooth Sink Service initialized");
        
        Utils::Logger::Info("Bluetooth Sink Service initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        Utils::Logger::Error("Exception during Bluetooth Sink Service initialization: " + std::string(e.what()));
        return false;
    }
}

void BluetoothSinkService::Shutdown() {
    if (!m_initialized) return;
    
    Utils::Logger::Info("Shutting down Bluetooth Sink Service...");
    
    // Stop service
    StopSinkService();
    
    // Disconnect all sources
    DisconnectAllSources();
    
    // Stop monitoring and service thread
    m_monitorTimer->Stop();
    m_serviceThread->Stop();
    
    // Cleanup A2DP sink
    CleanupA2DPSink();
    
    // Clear connected sources
    {
        std::lock_guard<std::mutex> lock(m_sourcesMutex);
        m_connectedSources.clear();
        m_sourceIndexMap.clear();
    }
    
    m_initialized = false;
    NotifyStatusChange("Bluetooth Sink Service shut down");
    
    Utils::Logger::Info("Bluetooth Sink Service shutdown complete");
}

bool BluetoothSinkService::StartSinkService() {
    if (!m_initialized || m_serviceRunning) return false;
    
    Utils::Logger::Info("Starting Bluetooth A2DP Sink Service...");
    
    try {
        // Start Bluetooth advertising
        if (!StartBluetoothAdvertising()) {
            Utils::Logger::Error("Failed to start Bluetooth advertising");
            return false;
        }
        
        m_serviceRunning = true;
        NotifyStatusChange("A2DP Sink Service running - Discoverable as: " + m_deviceName);
        
        // Start service loop
        m_serviceThread->PostTask([this]() { ServiceLoop(); });
        
        Utils::Logger::Info("Bluetooth A2DP Sink Service started successfully");
        Utils::Logger::Info("PC is now discoverable as: " + m_deviceName);
        
        return true;
    }
    catch (const std::exception& e) {
        Utils::Logger::Error("Exception starting Bluetooth Sink Service: " + std::string(e.what()));
        return false;
    }
}

bool BluetoothSinkService::StopSinkService() {
    if (!m_serviceRunning) return false;
    
    Utils::Logger::Info("Stopping Bluetooth A2DP Sink Service...");
    
    // Stop advertising
    StopBluetoothAdvertising();
    
    // Disconnect all current connections
    DisconnectAllSources();
    
    m_serviceRunning = false;
    NotifyStatusChange("A2DP Sink Service stopped");
    
    Utils::Logger::Info("Bluetooth A2DP Sink Service stopped");
    return true;
}

const std::vector<Models::ConnectedAudioSource>& BluetoothSinkService::GetConnectedSources() const {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    return m_connectedSources;
}

Models::ConnectedAudioSource* BluetoothSinkService::GetConnectedSource(const std::string& sourceId) {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    auto it = m_sourceIndexMap.find(sourceId);
    return it != m_sourceIndexMap.end() ? &m_connectedSources[it->second] : nullptr;
}

bool BluetoothSinkService::DisconnectSource(const std::string& sourceId) {
    Utils::Logger::Info("Disconnecting audio source: " + sourceId);
    
    auto source = GetConnectedSource(sourceId);
    if (!source) {
        Utils::Logger::Warn("Audio source not found: " + sourceId);
        return false;
    }
    
    // Stop audio stream if active
    if (source->isStreaming) {
        StopAudioStream(sourceId);
    }
    
    // Disconnect using Windows API
    if (!Platform::WindowsAPI::DisconnectBluetoothDevice(sourceId)) {
        Utils::Logger::Warn("Failed to disconnect via Windows API: " + sourceId);
    }
    Utils::Logger::Info("Bluetooth disconnection initiated for: " + sourceId);
    
    // Update source state
    source->state = Models::AudioSourceState::Disconnected;
    source->isStreaming = false;
    
    // Remove from audio mixer if connected
    if (m_audioMixer) {
        m_audioMixer->RemoveAudioSource(sourceId);
    }
    
    // Notify callbacks
    if (m_sourceDisconnectedCallback) {
        m_sourceDisconnectedCallback(*source);
    }
    
    // Remove from connected sources
    RemoveConnectedSource(sourceId);
    
    NotifyStatusChange("Disconnected from: " + source->name);
    Utils::Logger::Info("Successfully disconnected from: " + sourceId);
    
    return true;
}

bool BluetoothSinkService::DisconnectAllSources() {
    Utils::Logger::Info("Disconnecting all audio sources...");
    
    std::vector<std::string> sourceIds;
    {
        std::lock_guard<std::mutex> lock(m_sourcesMutex);
        for (const auto& source : m_connectedSources) {
            sourceIds.push_back(source.id);
        }
    }
    
    bool allSuccess = true;
    for (const auto& sourceId : sourceIds) {
        if (!DisconnectSource(sourceId)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

bool BluetoothSinkService::StartAudioStream(const std::string& sourceId) {
    Utils::Logger::Info("Starting audio stream for: " + sourceId);
    
    auto source = GetConnectedSource(sourceId);
    if (!source) return false;
    
    if (source->state != Models::AudioSourceState::Connected) {
        Utils::Logger::Warn("Cannot start stream - source not connected: " + sourceId);
        return false;
    }
    
    // Add source to audio mixer
    if (m_audioMixer) {
        if (!m_audioMixer->AddAudioSource(sourceId, source->streamInfo)) {
            Utils::Logger::Error("Failed to add source to audio mixer: " + sourceId);
            return false;
        }
    }
    
    // Update source state
    source->state = Models::AudioSourceState::Streaming;
    source->isStreaming = true;
    
    // Notify callbacks
    if (m_sourceUpdatedCallback) {
        m_sourceUpdatedCallback(*source);
    }
    
    m_activeStreams++;
    NotifyStatusChange("Streaming from: " + source->name);
    
    Utils::Logger::Info("Audio stream started for: " + sourceId);
    return true;
}

bool BluetoothSinkService::StopAudioStream(const std::string& sourceId) {
    Utils::Logger::Info("Stopping audio stream for: " + sourceId);
    
    auto source = GetConnectedSource(sourceId);
    if (!source) return false;
    
    // Remove from audio mixer
    if (m_audioMixer) {
        m_audioMixer->RemoveAudioSource(sourceId);
    }
    
    // Update source state
    source->state = Models::AudioSourceState::Connected;
    source->isStreaming = false;
    
    // Notify callbacks
    if (m_sourceUpdatedCallback) {
        m_sourceUpdatedCallback(*source);
    }
    
    if (m_activeStreams > 0) m_activeStreams--;
    
    Utils::Logger::Info("Audio stream stopped for: " + sourceId);
    return true;
}

bool BluetoothSinkService::SetSourceVolume(const std::string& sourceId, float volume) {
    auto source = GetConnectedSource(sourceId);
    if (!source) return false;
    
    source->volume = std::clamp(volume, 0.0f, 1.0f);
    
    // Update in audio mixer
    if (m_audioMixer) {
        m_audioMixer->SetSourceVolume(sourceId, source->volume);
    }
    
    // Notify callbacks
    if (m_sourceUpdatedCallback) {
        m_sourceUpdatedCallback(*source);
    }
    
    return true;
}

bool BluetoothSinkService::SetSourceMute(const std::string& sourceId, bool muted) {
    auto source = GetConnectedSource(sourceId);
    if (!source) return false;
    
    source->isMuted = muted;
    
    // Update in audio mixer
    if (m_audioMixer) {
        m_audioMixer->SetSourceMute(sourceId, muted);
    }
    
    // Notify callbacks
    if (m_sourceUpdatedCallback) {
        m_sourceUpdatedCallback(*source);
    }
    
    return true;
}

bool BluetoothSinkService::SetDeviceName(const std::string& name) {
    m_deviceName = name;
    
    // TODO: Update Bluetooth advertising with new name
    Utils::Logger::Info("Device name set to: " + name);
    
    NotifyStatusChange("Device name changed to: " + name);
    return true;
}

bool BluetoothSinkService::SetDiscoverable(bool discoverable) {
    m_isDiscoverable = discoverable;
    
    // TODO: Update Bluetooth discoverability
    Utils::Logger::Info("Discoverability set to: " + std::string(discoverable ? "true" : "false"));
    
    return true;
}

std::string BluetoothSinkService::GetDeviceName() const {
    return m_deviceName;
}

size_t BluetoothSinkService::GetConnectedSourceCount() const {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    return m_connectedSources.size();
}

size_t BluetoothSinkService::GetStreamingSourceCount() const {
    return m_activeStreams.load();
}

std::string BluetoothSinkService::GetServiceStatus() const {
    std::lock_guard<std::mutex> lock(m_statusMutex);
    return m_currentStatus;
}

bool BluetoothSinkService::HasConnectedSources() const {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    return !m_connectedSources.empty();
}

void BluetoothSinkService::SetAudioMixer(std::shared_ptr<AudioMixer> mixer) {
    m_audioMixer = mixer;
    Utils::Logger::Info("Audio mixer connected to Bluetooth Sink Service");
}

// Private methods
void BluetoothSinkService::ServiceLoop() {
    Utils::Logger::Info("Bluetooth Sink Service loop started");
    
    while (m_serviceRunning) {
        try {
            HandleIncomingConnections();
            
            // Small delay to prevent 100% CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        catch (const std::exception& e) {
            Utils::Logger::Error("Exception in service loop: " + std::string(e.what()));
        }
    }
    
    Utils::Logger::Info("Bluetooth Sink Service loop ended");
}

void BluetoothSinkService::HandleIncomingConnections() {
    // Poll for incoming A2DP connections using Windows Bluetooth APIs
    try {
        // Get connected audio sources from Windows API
        auto connectedDevices = Platform::WindowsAPI::GetConnectedAudioSources();
        
        // Check for new connections
        for (const auto& deviceId : connectedDevices) {
            if (GetConnectedSource(deviceId) == nullptr) {
                // New connection detected
                Utils::Logger::Info("New A2DP connection detected: " + deviceId);
                OnIncomingConnection(deviceId, "Unknown Device");
            }
        }
        
        // Check for disconnected devices
        std::vector<std::string> toRemove;
        {
            std::lock_guard<std::mutex> lock(m_sourcesMutex);
            for (const auto& source : m_connectedSources) {
                if (std::find(connectedDevices.begin(), connectedDevices.end(), source.id) == connectedDevices.end()) {
                    toRemove.push_back(source.id);
                }
            }
        }
        
        for (const auto& deviceId : toRemove) {
            Utils::Logger::Info("A2DP disconnection detected: " + deviceId);
            OnConnectionLost(deviceId);
        }
    }
    catch (const std::exception& e) {
        Utils::Logger::Error("Exception in HandleIncomingConnections: " + std::string(e.what()));
    }
}

void BluetoothSinkService::MonitorConnections() {
    if (!m_serviceRunning) return;
    
    // Monitor connection health and update statistics
    auto now = std::chrono::system_clock::now();
    
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    for (auto& source : m_connectedSources) {
        // Update last activity and check for timeouts
        auto timeSinceActivity = std::chrono::duration_cast<std::chrono::seconds>(now - source.lastActivity);
        
        if (timeSinceActivity.count() > 30 && source.state == Models::AudioSourceState::Streaming) {
            // No activity for 30 seconds - pause stream
            source.state = Models::AudioSourceState::Paused;
            source.isStreaming = false;
            
            Utils::Logger::Warn("Audio stream paused due to inactivity: " + source.name);
            
            if (m_sourceUpdatedCallback) {
                m_sourceUpdatedCallback(source);
            }
        }
    }
}

void BluetoothSinkService::OnIncomingConnection(const std::string& deviceId, const std::string& deviceName) {
    Utils::Logger::Info("Incoming connection from: " + deviceName + " (" + deviceId + ")");
    
    // Create new audio source
    Models::ConnectedAudioSource source;
    source.id = deviceId;
    source.name = deviceName;
    source.deviceType = "Unknown";  // Could be determined from device class
    source.state = Models::AudioSourceState::Connecting;
    source.connectedAt = std::chrono::system_clock::now();
    source.lastActivity = source.connectedAt;
    
    // Add to connected sources
    AddConnectedSource(source);
    
    // Automatically accept connection (could be made configurable)
    AcceptConnection(deviceId);
    
    NotifyStatusChange("Incoming connection from: " + deviceName);
}

void BluetoothSinkService::OnConnectionEstablished(const std::string& sourceId) {
    auto source = GetConnectedSource(sourceId);
    if (!source) return;
    
    source->state = Models::AudioSourceState::Connected;
    source->lastActivity = std::chrono::system_clock::now();
    
    Utils::Logger::Info("Connection established with: " + source->name);
    
    if (m_sourceConnectedCallback) {
        m_sourceConnectedCallback(*source);
    }
    
    m_totalConnections++;
    NotifyStatusChange("Connected to: " + source->name);
}

void BluetoothSinkService::OnConnectionLost(const std::string& sourceId) {
    Utils::Logger::Info("Connection lost with: " + sourceId);
    
    auto source = GetConnectedSource(sourceId);
    if (!source) return;
    
    // Stop audio stream if active
    if (source->isStreaming) {
        StopAudioStream(sourceId);
    }
    
    // Update source state
    source->state = Models::AudioSourceState::Disconnected;
    source->isStreaming = false;
    
    // Remove from audio mixer if connected
    if (m_audioMixer) {
        m_audioMixer->RemoveAudioSource(sourceId);
    }
    
    // Notify callbacks
    if (m_sourceDisconnectedCallback) {
        m_sourceDisconnectedCallback(*source);
    }
    
    // Remove from connected sources
    RemoveConnectedSource(sourceId);
    
    NotifyStatusChange("Connection lost with: " + source->name);
    Utils::Logger::Info("Successfully handled connection loss for: " + sourceId);
}

void BluetoothSinkService::OnAudioDataReceived(const std::string& sourceId, const float* audioData, size_t sampleCount) {
    // Process incoming audio data
    if (m_audioMixer && audioData && sampleCount > 0) {
        m_audioMixer->ProcessAudioData(sourceId, audioData, sampleCount);
    }
    
    // Update last activity
    auto source = GetConnectedSource(sourceId);
    if (source) {
        source->lastActivity = std::chrono::system_clock::now();
        
        // Ensure source is in streaming state
        if (source->state != Models::AudioSourceState::Streaming) {
            source->state = Models::AudioSourceState::Streaming;
            source->isStreaming = true;
            
            if (m_sourceUpdatedCallback) {
                m_sourceUpdatedCallback(*source);
            }
        }
    }
    
    // Forward to audio data callback if registered
    if (m_audioDataCallback) {
        m_audioDataCallback(sourceId, audioData, sampleCount);
    }
}

void BluetoothSinkService::AddConnectedSource(const Models::ConnectedAudioSource& source) {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    
    m_sourceIndexMap[source.id] = m_connectedSources.size();
    m_connectedSources.push_back(source);
    
    Utils::Logger::Info("Added connected source: " + source.name + " (" + source.id + ")");
}

void BluetoothSinkService::RemoveConnectedSource(const std::string& sourceId) {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    
    auto it = m_sourceIndexMap.find(sourceId);
    if (it != m_sourceIndexMap.end()) {
        size_t index = it->second;
        
        m_connectedSources.erase(m_connectedSources.begin() + index);
        m_sourceIndexMap.erase(it);
        
        // Update indices for remaining sources
        for (size_t i = index; i < m_connectedSources.size(); ++i) {
            m_sourceIndexMap[m_connectedSources[i].id] = i;
        }
        
        Utils::Logger::Info("Removed connected source: " + sourceId);
    }
}

void BluetoothSinkService::NotifyStatusChange(const std::string& status) {
    {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_currentStatus = status;
    }
    
    if (m_statusCallback) {
        m_statusCallback(status);
    }
}

bool BluetoothSinkService::StartBluetoothAdvertising() {
    Utils::Logger::Info("Starting Bluetooth A2DP advertising...");
    
    // Start the actual A2DP sink service using Windows API
    if (!Platform::WindowsAPI::StartBluetoothA2DPSink(m_deviceName)) {
        Utils::Logger::Error("Failed to start Windows Bluetooth A2DP Sink");
        return false;
    }
    
    // Set up audio stream callback to receive decoded audio data
    Platform::WindowsAPI::SetAudioStreamCallback(
        [this](const std::string& deviceId, const float* audioData, size_t sampleCount) {
            OnAudioDataReceived(deviceId, audioData, sampleCount);
        });
    
    // Set up connection callback to handle device connections/disconnections
    Platform::WindowsAPI::SetBluetoothConnectionCallback(
        [this](const std::string& deviceId, const std::string& deviceName, bool connected) {
            if (connected) {
                OnIncomingConnection(deviceId, deviceName);
            } else {
                OnConnectionLost(deviceId);
            }
        });
    
    Utils::Logger::Info("Bluetooth A2DP advertising started successfully");
    return true;
}

bool BluetoothSinkService::StopBluetoothAdvertising() {
    Utils::Logger::Info("Stopping Bluetooth A2DP advertising...");
    
    // Stop the actual A2DP sink service using Windows API
    if (!Platform::WindowsAPI::StopBluetoothA2DPSink()) {
        Utils::Logger::Error("Failed to stop Windows Bluetooth A2DP Sink");
        return false;
    }
    
    Utils::Logger::Info("Bluetooth A2DP advertising stopped successfully");
    return true;
}

bool BluetoothSinkService::InitializeA2DPSink() {
    // TODO: Initialize Windows A2DP sink service
    Utils::Logger::Info("A2DP Sink initialized (simplified implementation)");
    return true;
}

void BluetoothSinkService::CleanupA2DPSink() {
    // TODO: Cleanup A2DP sink resources
    Utils::Logger::Info("A2DP Sink cleaned up");
}

bool BluetoothSinkService::AcceptConnection(const std::string& sourceId) {
    Utils::Logger::Info("Accepting connection from: " + sourceId);
    
    // Simulate connection acceptance
    OnConnectionEstablished(sourceId);
    
    return true;
}

bool BluetoothSinkService::RejectConnection(const std::string& sourceId) {
    Utils::Logger::Info("Rejecting connection from: " + sourceId);
    
    // Remove the connecting source
    RemoveConnectedSource(sourceId);
    
    return true;
}

// Callback setters
void BluetoothSinkService::SetSourceConnectedCallback(SourceCallback callback) {
    m_sourceConnectedCallback = callback;
}

void BluetoothSinkService::SetSourceDisconnectedCallback(SourceCallback callback) {
    m_sourceDisconnectedCallback = callback;
}

void BluetoothSinkService::SetSourceUpdatedCallback(SourceCallback callback) {
    m_sourceUpdatedCallback = callback;
}

void BluetoothSinkService::SetStatusCallback(StatusCallback callback) {
    m_statusCallback = callback;
}

void BluetoothSinkService::SetAudioDataCallback(AudioDataCallback callback) {
    m_audioDataCallback = callback;
}

} // namespace BluetoothAudio 
