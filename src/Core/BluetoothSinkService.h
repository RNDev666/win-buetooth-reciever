#pragma once
#include "../Models/ConnectedAudioSource.h"
#include "../Utils/Threading.h"
#include "AudioMixer.h"
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace BluetoothAudio {

class BluetoothSinkService {
public:
    using SourceCallback = std::function<void(const Models::ConnectedAudioSource&)>;
    using StatusCallback = std::function<void(const std::string&)>;
    using AudioDataCallback = std::function<void(const std::string&, const float*, size_t)>;
    
    BluetoothSinkService();
    ~BluetoothSinkService();
    
    bool Initialize();
    void Shutdown();
    
    // A2DP Sink Service Control
    bool StartSinkService();
    bool StopSinkService();
    bool IsServiceRunning() const { return m_serviceRunning; }
    
    // Device Management (Incoming Connections)
    const std::vector<Models::ConnectedAudioSource>& GetConnectedSources() const;
    Models::ConnectedAudioSource* GetConnectedSource(const std::string& sourceId);
    
    // Connection Management
    bool DisconnectSource(const std::string& sourceId);
    bool DisconnectAllSources();
    bool AcceptConnection(const std::string& sourceId);
    bool RejectConnection(const std::string& sourceId);
    
    // Audio Stream Control
    bool StartAudioStream(const std::string& sourceId);
    bool StopAudioStream(const std::string& sourceId);
    bool SetSourceVolume(const std::string& sourceId, float volume);
    bool SetSourceMute(const std::string& sourceId, bool muted);
    
    // Service Configuration
    bool SetDeviceName(const std::string& name);
    bool SetDiscoverable(bool discoverable);
    bool SetPairable(bool pairable);
    std::string GetDeviceName() const;
    
    // Connection State
    bool HasConnectedSources() const;
    size_t GetConnectedSourceCount() const;
    size_t GetStreamingSourceCount() const;
    std::string GetServiceStatus() const;
    
    // Callbacks
    void SetSourceConnectedCallback(SourceCallback callback);
    void SetSourceDisconnectedCallback(SourceCallback callback);
    void SetSourceUpdatedCallback(SourceCallback callback);
    void SetStatusCallback(StatusCallback callback);
    void SetAudioDataCallback(AudioDataCallback callback);
    
    // Audio Mixer Integration
    void SetAudioMixer(std::shared_ptr<AudioMixer> mixer);
    
private:
    // Service Management
    void ServiceLoop();
    void HandleIncomingConnections();
    void MonitorConnections();
    
    // Connection Handling
    void OnIncomingConnection(const std::string& deviceId, const std::string& deviceName);
    void OnConnectionEstablished(const std::string& sourceId);
    void OnConnectionLost(const std::string& sourceId);
    void OnAudioStreamStarted(const std::string& sourceId, const Models::AudioStreamInfo& streamInfo);
    void OnAudioStreamStopped(const std::string& sourceId);
    void OnAudioDataReceived(const std::string& sourceId, const float* audioData, size_t sampleCount);
    
    // Source Management
    void AddConnectedSource(const Models::ConnectedAudioSource& source);
    void RemoveConnectedSource(const std::string& sourceId);
    void UpdateConnectedSource(const Models::ConnectedAudioSource& source);
    void NotifyStatusChange(const std::string& status);
    
    // Bluetooth Service Operations
    bool StartBluetoothAdvertising();
    bool StopBluetoothAdvertising();
    bool InitializeA2DPSink();
    void CleanupA2DPSink();
    
    // Audio Processing
    void ProcessIncomingAudio(const std::string& sourceId, const void* audioData, 
                             size_t dataSize, const Models::AudioStreamInfo& streamInfo);
    void DecodeAudioStream(const void* encodedData, size_t dataSize, 
                          const Models::AudioStreamInfo& streamInfo, 
                          std::vector<float>& decodedSamples);
    
    // Member variables
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_serviceRunning{false};
    std::atomic<bool> m_isDiscoverable{true};
    std::atomic<bool> m_isPairable{true};
    
    std::unique_ptr<Utils::WorkerThread> m_serviceThread;
    std::unique_ptr<Utils::Timer> m_monitorTimer;
    
    mutable std::mutex m_sourcesMutex;
    std::vector<Models::ConnectedAudioSource> m_connectedSources;
    std::unordered_map<std::string, size_t> m_sourceIndexMap;
    
    mutable std::mutex m_statusMutex;
    std::string m_currentStatus;
    std::string m_deviceName;
    
    // Audio processing
    std::shared_ptr<AudioMixer> m_audioMixer;
    std::vector<float> m_tempAudioBuffer;
    
    // Bluetooth service data
    void* m_bluetoothService = nullptr;     // Platform-specific service handle
    void* m_a2dpSink = nullptr;             // A2DP sink interface
    
    // Callbacks
    SourceCallback m_sourceConnectedCallback;
    SourceCallback m_sourceDisconnectedCallback;
    SourceCallback m_sourceUpdatedCallback;
    StatusCallback m_statusCallback;
    AudioDataCallback m_audioDataCallback;
    
    // Statistics
    std::atomic<size_t> m_totalConnections{0};
    std::atomic<size_t> m_activeStreams{0};
    
    // Configuration
    struct ServiceConfig {
        int maxConnections = 4;         // Maximum simultaneous connections
        bool requirePairing = true;     // Require device pairing
        bool autoAcceptConnections = false;  // Auto-accept known devices
        std::vector<std::string> allowedDevices;  // Whitelist of device IDs
        std::vector<std::string> blockedDevices;  // Blacklist of device IDs
    } m_config;
    
    // Constants
    static constexpr int DEFAULT_A2DP_PORT = 25;
    static constexpr int SERVICE_MONITOR_INTERVAL_MS = 1000;
    static constexpr size_t AUDIO_BUFFER_SIZE = 4096;
};

} // namespace BluetoothAudio 
