#pragma once
#include "../Models/MixingState.h"
#include "../Models/ConnectedAudioSource.h"
#include "../Utils/Threading.h"
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <unordered_map>

namespace BluetoothAudio {

class AudioMixer {
public:
    using MixingCallback = std::function<void(const Models::MixingState&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    AudioMixer();
    ~AudioMixer();
    
    bool Initialize();
    void Shutdown();
    
    // Mixing control
    bool StartMixing();
    bool StopMixing();
    bool IsRunning() const { return m_isRunning; }
    
    // Source management
    bool AddAudioSource(const std::string& sourceId, const Models::AudioStreamInfo& streamInfo);
    bool RemoveAudioSource(const std::string& sourceId);
    bool UpdateSourceInfo(const std::string& sourceId, const Models::AudioStreamInfo& streamInfo);
    
    // Audio data processing
    bool ProcessAudioData(const std::string& sourceId, const float* audioData, size_t sampleCount);
    
    // Volume and mixing controls
    bool SetSourceVolume(const std::string& sourceId, float volume);
    bool SetSourceMute(const std::string& sourceId, bool muted);
    bool SetMasterVolume(float volume);
    bool SetMixingMode(Models::MixingMode mode);
    bool SetPCAudioEnabled(bool enabled);
    
    // Configuration
    const Models::MixingState& GetMixingState() const;
    bool UpdateMixingConfiguration(const Models::MixingConfiguration& config);
    
    // Audio device management
    bool SetOutputDevice(const std::string& deviceId);
    std::string GetCurrentOutputDevice() const;
    
    // Callbacks
    void SetMixingCallback(MixingCallback callback);
    void SetErrorCallback(ErrorCallback callback);
    
    // Statistics
    float GetCurrentLatency() const;
    float GetCpuUsage() const;
    int GetActiveSourceCount() const;
    std::vector<std::string> GetActiveSources() const;
    
private:
    struct AudioSource {
        std::string id;
        Models::AudioStreamInfo streamInfo;
        Models::AudioSourceMixSettings mixSettings;
        
        // Audio buffers
        std::vector<float> inputBuffer;
        std::vector<float> processedBuffer;
        size_t bufferPosition = 0;
        
        // Statistics
        float currentLevel = 0.0f;
        int dropoutCount = 0;
        std::chrono::steady_clock::time_point lastDataTime;
        
        bool isActive = false;
    };
    
    // Core mixing methods
    void MixingLoop();
    void ProcessMixing();
    bool InitializeAudioOutput();
    void CleanupAudioOutput();
    
    // Audio processing
    void MixAudioSources(float* outputBuffer, size_t sampleCount);
    void ApplyVolumeAndEffects(AudioSource& source, float* buffer, size_t sampleCount);
    void ConvertSampleRate(const float* input, float* output, size_t inputSamples, 
                          int inputRate, int outputRate);
    
    // Buffer management
    bool EnsureBufferSize(AudioSource& source, size_t requiredSize);
    void UpdateSourceStatistics(AudioSource& source);
    
    // Configuration helpers
    void ApplyMixingConfiguration();
    void NotifyMixingStateChanged();
    void NotifyError(const std::string& error);
    
    // Member variables
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_isRunning{false};
    std::unique_ptr<Utils::WorkerThread> m_mixingThread;
    
    mutable std::mutex m_sourcesMutex;
    std::unordered_map<std::string, std::unique_ptr<AudioSource>> m_audioSources;
    
    mutable std::mutex m_stateMutex;
    Models::MixingState m_mixingState;
    
    // Audio output
    std::string m_outputDeviceId;
    void* m_audioClient = nullptr;          // IAudioClient*
    void* m_renderClient = nullptr;         // IAudioRenderClient*
    
    // Processing buffers
    std::vector<float> m_mixBuffer;
    std::vector<float> m_tempBuffer;
    
    // Performance monitoring
    std::chrono::steady_clock::time_point m_lastMixTime;
    float m_avgProcessingTime = 0.0f;
    
    // Callbacks
    MixingCallback m_mixingCallback;
    ErrorCallback m_errorCallback;
    
    // Constants
    static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;
    static constexpr int DEFAULT_SAMPLE_RATE = 48000;
    static constexpr float SILENCE_THRESHOLD = 0.001f;
};

} // namespace BluetoothAudio 
