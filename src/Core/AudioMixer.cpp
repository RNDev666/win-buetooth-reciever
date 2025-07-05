#include "AudioMixer.h"
#include "../Platform/WindowsAPI.h"
#include "../Utils/Logger.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

namespace BluetoothAudio {

AudioMixer::AudioMixer() 
    : m_mixingThread(std::make_unique<Utils::WorkerThread>()) {
    
    // Initialize mixing state with default configuration
    m_mixingState.config.mode = Models::MixingMode::Mix;
    m_mixingState.config.masterVolume = 1.0f;
    m_mixingState.config.pcAudioVolume = 1.0f;
    m_mixingState.config.bufferSize = DEFAULT_BUFFER_SIZE;
    m_mixingState.config.outputSampleRate = DEFAULT_SAMPLE_RATE;
    m_mixingState.config.outputChannels = 2;
    m_mixingState.status = "Stopped";
}

AudioMixer::~AudioMixer() {
    Shutdown();
}

bool AudioMixer::Initialize() {
    if (m_initialized) return true;
    
    Utils::Logger::Info("Initializing Audio Mixer...");
    
    try {
        // Initialize audio output
        if (!InitializeAudioOutput()) {
            Utils::Logger::Error("Failed to initialize audio output");
            return false;
        }
        
        // Start mixing thread
        m_mixingThread->Start();
        
        // Initialize buffers
        m_mixBuffer.resize(m_mixingState.config.bufferSize * m_mixingState.config.outputChannels);
        m_tempBuffer.resize(m_mixBuffer.size());
        
        m_initialized = true;
        m_mixingState.status = "Ready";
        
        Utils::Logger::Info("Audio Mixer initialized successfully");
        NotifyMixingStateChanged();
        
        return true;
    }
    catch (const std::exception& e) {
        Utils::Logger::Error("Exception during Audio Mixer initialization: " + std::string(e.what()));
        return false;
    }
}

void AudioMixer::Shutdown() {
    if (!m_initialized) return;
    
    Utils::Logger::Info("Shutting down Audio Mixer...");
    
    StopMixing();
    
    // Stop mixing thread
    m_mixingThread->Stop();
    
    // Clear audio sources
    {
        std::lock_guard<std::mutex> lock(m_sourcesMutex);
        m_audioSources.clear();
    }
    
    // Cleanup audio output
    CleanupAudioOutput();
    
    m_initialized = false;
    m_mixingState.status = "Stopped";
    
    Utils::Logger::Info("Audio Mixer shutdown complete");
}

bool AudioMixer::StartMixing() {
    if (!m_initialized || m_isRunning) return false;
    
    Utils::Logger::Info("Starting audio mixing...");
    
    m_isRunning = true;
    m_mixingState.isEnabled = true;
    m_mixingState.isProcessing = true;
    m_mixingState.status = "Running";
    
    // Start mixing loop in background thread
    m_mixingThread->PostTask([this]() { MixingLoop(); });
    
    NotifyMixingStateChanged();
    return true;
}

bool AudioMixer::StopMixing() {
    if (!m_isRunning) return false;
    
    Utils::Logger::Info("Stopping audio mixing...");
    
    m_isRunning = false;
    m_mixingState.isEnabled = false;
    m_mixingState.isProcessing = false;
    m_mixingState.status = "Stopped";
    
    NotifyMixingStateChanged();
    return true;
}

bool AudioMixer::AddAudioSource(const std::string& sourceId, const Models::AudioStreamInfo& streamInfo) {
    Utils::Logger::Info("Adding audio source: " + sourceId);
    
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    
    if (m_audioSources.find(sourceId) != m_audioSources.end()) {
        Utils::Logger::Warn("Audio source already exists: " + sourceId);
        return false;
    }
    
    auto source = std::make_unique<AudioSource>();
    source->id = sourceId;
    source->streamInfo = streamInfo;
    source->mixSettings.sourceId = sourceId;
    source->lastDataTime = std::chrono::steady_clock::now();
    
    // Initialize buffers based on stream info
    size_t bufferSize = (streamInfo.sampleRate * m_mixingState.config.outputChannels * 100) / 1000; // 100ms buffer
    source->inputBuffer.resize(bufferSize, 0.0f);
    source->processedBuffer.resize(bufferSize, 0.0f);
    
    m_audioSources[sourceId] = std::move(source);
    
    // Update mixing state
    {
        std::lock_guard<std::mutex> stateLock(m_stateMutex);
        m_mixingState.GetOrCreateSourceSettings(sourceId);
    }
    
    Utils::Logger::Info("Audio source added successfully: " + sourceId);
    NotifyMixingStateChanged();
    
    return true;
}

bool AudioMixer::RemoveAudioSource(const std::string& sourceId) {
    Utils::Logger::Info("Removing audio source: " + sourceId);
    
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    
    auto it = m_audioSources.find(sourceId);
    if (it == m_audioSources.end()) {
        Utils::Logger::Warn("Audio source not found: " + sourceId);
        return false;
    }
    
    m_audioSources.erase(it);
    
    // Update mixing state
    {
        std::lock_guard<std::mutex> stateLock(m_stateMutex);
        m_mixingState.RemoveSourceSettings(sourceId);
    }
    
    Utils::Logger::Info("Audio source removed: " + sourceId);
    NotifyMixingStateChanged();
    
    return true;
}

bool AudioMixer::ProcessAudioData(const std::string& sourceId, const float* audioData, size_t sampleCount) {
    if (!m_isRunning || !audioData || sampleCount == 0) return false;
    
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    
    auto it = m_audioSources.find(sourceId);
    if (it == m_audioSources.end()) return false;
    
    auto& source = *it->second;
    
    // Ensure buffer has enough space
    if (!EnsureBufferSize(source, sampleCount)) {
        return false;
    }
    
    // Copy audio data to source buffer
    std::memcpy(source.inputBuffer.data(), audioData, sampleCount * sizeof(float));
    source.bufferPosition = sampleCount;
    source.lastDataTime = std::chrono::steady_clock::now();
    source.isActive = true;
    
    // Update statistics
    UpdateSourceStatistics(source);
    
    return true;
}

bool AudioMixer::SetSourceVolume(const std::string& sourceId, float volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    
    std::lock_guard<std::mutex> stateLock(m_stateMutex);
    auto& settings = m_mixingState.GetOrCreateSourceSettings(sourceId);
    settings.volume = volume;
    
    NotifyMixingStateChanged();
    return true;
}

bool AudioMixer::SetSourceMute(const std::string& sourceId, bool muted) {
    std::lock_guard<std::mutex> stateLock(m_stateMutex);
    auto& settings = m_mixingState.GetOrCreateSourceSettings(sourceId);
    settings.isMuted = muted;
    
    NotifyMixingStateChanged();
    return true;
}

bool AudioMixer::SetMasterVolume(float volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    
    std::lock_guard<std::mutex> stateLock(m_stateMutex);
    m_mixingState.config.masterVolume = volume;
    
    NotifyMixingStateChanged();
    return true;
}

bool AudioMixer::SetMixingMode(Models::MixingMode mode) {
    std::lock_guard<std::mutex> stateLock(m_stateMutex);
    m_mixingState.config.mode = mode;
    
    NotifyMixingStateChanged();
    return true;
}

bool AudioMixer::SetPCAudioEnabled(bool enabled) {
    std::lock_guard<std::mutex> stateLock(m_stateMutex);
    m_mixingState.config.mutePCAudio = !enabled;
    
    NotifyMixingStateChanged();
    return true;
}

const Models::MixingState& AudioMixer::GetMixingState() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_mixingState;
}

int AudioMixer::GetActiveSourceCount() const {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    int count = 0;
    for (const auto& [id, source] : m_audioSources) {
        if (source->isActive) count++;
    }
    return count;
}

// Private methods
void AudioMixer::MixingLoop() {
    Utils::Logger::Info("Audio mixing loop started");
    
    while (m_isRunning) {
        try {
            ProcessMixing();
            
            // Small delay to prevent 100% CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        catch (const std::exception& e) {
            Utils::Logger::Error("Exception in mixing loop: " + std::string(e.what()));
            NotifyError("Mixing loop error: " + std::string(e.what()));
        }
    }
    
    Utils::Logger::Info("Audio mixing loop ended");
}

void AudioMixer::ProcessMixing() {
    auto startTime = std::chrono::steady_clock::now();
    
    // Clear mix buffer
    std::fill(m_mixBuffer.begin(), m_mixBuffer.end(), 0.0f);
    
    // Process and mix all audio sources
    MixAudioSources(m_mixBuffer.data(), m_mixingState.config.bufferSize);
    
    // Output mixed audio to speakers via WASAPI
    if (m_audioClient && m_renderClient) {
        IAudioClient* audioClient = static_cast<IAudioClient*>(m_audioClient);
        IAudioRenderClient* renderClient = static_cast<IAudioRenderClient*>(m_renderClient);
        
        // Get available buffer space
        UINT32 numFramesPadding;
        HRESULT hr = audioClient->GetCurrentPadding(&numFramesPadding);
        if (SUCCEEDED(hr)) {
            UINT32 bufferFrameCount = m_mixingState.config.bufferSize;
            UINT32 availableFrames = bufferFrameCount - numFramesPadding;
            
            if (availableFrames > 0) {
                // Get buffer from render client
                BYTE* audioBuffer = nullptr;
                hr = renderClient->GetBuffer(availableFrames, &audioBuffer);
                if (SUCCEEDED(hr)) {
                    // Convert float samples to 16-bit PCM
                    INT16* outputBuffer = reinterpret_cast<INT16*>(audioBuffer);
                    size_t samplesToWrite = std::min(static_cast<size_t>(availableFrames) * 2, m_mixBuffer.size());
                    
                    for (size_t i = 0; i < samplesToWrite; ++i) {
                        float sample = std::clamp(m_mixBuffer[i], -1.0f, 1.0f);
                        outputBuffer[i] = static_cast<INT16>(sample * 32767.0f);
                    }
                    
                    // Release buffer
                    renderClient->ReleaseBuffer(availableFrames, 0);
                }
            }
        }
    }
    
    // Update performance statistics
    auto endTime = std::chrono::steady_clock::now();
    auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Update mixing statistics
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_mixingState.stats.currentLatency = static_cast<float>(processingTime.count()) / 1000.0f;
        m_mixingState.activeSourceCount = GetActiveSourceCount();
    }
}

void AudioMixer::MixAudioSources(float* outputBuffer, size_t sampleCount) {
    std::lock_guard<std::mutex> lock(m_sourcesMutex);
    
    for (const auto& [id, source] : m_audioSources) {
        if (!source->isActive || source->bufferPosition == 0) continue;
        
        // Get mixing settings for this source
        Models::AudioSourceMixSettings settings;
        {
            std::lock_guard<std::mutex> stateLock(m_stateMutex);
            auto& sourceSettings = m_mixingState.GetOrCreateSourceSettings(id);
            settings = sourceSettings;
        }
        
        if (settings.isMuted || !settings.isEnabled) continue;
        
        // Process source audio with effects and volume
        ApplyVolumeAndEffects(*source, source->inputBuffer.data(), 
                             std::min(sampleCount, source->bufferPosition));
        
        // Mix into output buffer
        size_t samplesToMix = std::min(sampleCount, source->bufferPosition);
        for (size_t i = 0; i < samplesToMix; ++i) {
            outputBuffer[i] += source->processedBuffer[i] * settings.volume;
        }
        
        // Update source level for UI display
        {
            std::lock_guard<std::mutex> stateLock(m_stateMutex);
            m_mixingState.UpdateSourceLevel(id, source->currentLevel);
        }
    }
    
    // Apply master volume
    float masterVol = m_mixingState.config.masterVolume;
    for (size_t i = 0; i < sampleCount; ++i) {
        outputBuffer[i] *= masterVol;
    }
}

void AudioMixer::ApplyVolumeAndEffects(AudioSource& source, float* buffer, size_t sampleCount) {
    // Calculate current audio level (RMS)
    float sum = 0.0f;
    for (size_t i = 0; i < sampleCount; ++i) {
        sum += buffer[i] * buffer[i];
    }
    source.currentLevel = std::sqrt(sum / sampleCount);
    
    // Copy to processed buffer (effects would go here)
    std::memcpy(source.processedBuffer.data(), buffer, sampleCount * sizeof(float));
}

bool AudioMixer::EnsureBufferSize(AudioSource& source, size_t requiredSize) {
    if (source.inputBuffer.size() < requiredSize) {
        source.inputBuffer.resize(requiredSize * 2); // Grow with headroom
        source.processedBuffer.resize(source.inputBuffer.size());
    }
    return true;
}

void AudioMixer::UpdateSourceStatistics(AudioSource& source) {
    // Update packet statistics, detect dropouts, etc.
    // This is a simplified implementation
}

bool AudioMixer::InitializeAudioOutput() {
    Utils::Logger::Info("Initializing WASAPI audio output...");
    
    try {
        // Get default audio device
        IMMDeviceEnumerator* deviceEnumerator = nullptr;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, 
                                     __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to create device enumerator");
            return false;
        }
        
        IMMDevice* defaultDevice = nullptr;
        hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
        deviceEnumerator->Release();
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to get default audio endpoint");
            return false;
        }
        
        // Initialize audio client
        IAudioClient* audioClient = nullptr;
        hr = defaultDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audioClient);
        defaultDevice->Release();
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to activate audio client");
            return false;
        }
        
        // Set up audio format (48kHz, 16-bit, stereo)
        WAVEFORMATEX waveFormat = {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 2;
        waveFormat.nSamplesPerSec = 48000;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;
        
        // Initialize audio client in shared mode
        REFERENCE_TIME requestedDuration = 200000; // 20ms buffer
        hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                    AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                    requestedDuration, 0, &waveFormat, nullptr);
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to initialize audio client");
            audioClient->Release();
            return false;
        }
        
        // Get buffer size
        UINT32 bufferFrameCount;
        hr = audioClient->GetBufferSize(&bufferFrameCount);
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to get buffer size");
            audioClient->Release();
            return false;
        }
        
        // Get render client
        IAudioRenderClient* renderClient = nullptr;
        hr = audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&renderClient);
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to get render client");
            audioClient->Release();
            return false;
        }
        
        // Create audio event
        HANDLE audioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (audioEvent == nullptr) {
            Utils::Logger::Error("Failed to create audio event");
            renderClient->Release();
            audioClient->Release();
            return false;
        }
        
        // Set event handle
        hr = audioClient->SetEventHandle(audioEvent);
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to set event handle");
            CloseHandle(audioEvent);
            renderClient->Release();
            audioClient->Release();
            return false;
        }
        
        // Start audio client
        hr = audioClient->Start();
        if (FAILED(hr)) {
            Utils::Logger::Error("Failed to start audio client");
            CloseHandle(audioEvent);
            renderClient->Release();
            audioClient->Release();
            return false;
        }
        
        // Store audio client and render client
        m_audioClient = audioClient;
        m_renderClient = renderClient;
        
        // Update mixing state with actual audio format
        m_mixingState.config.outputSampleRate = waveFormat.nSamplesPerSec;
        m_mixingState.config.outputChannels = waveFormat.nChannels;
        m_mixingState.config.bufferSize = bufferFrameCount;
        
        Utils::Logger::Info("WASAPI audio output initialized successfully");
        Utils::Logger::Info("Sample Rate: " + std::to_string(waveFormat.nSamplesPerSec));
        Utils::Logger::Info("Channels: " + std::to_string(waveFormat.nChannels));
        Utils::Logger::Info("Buffer Size: " + std::to_string(bufferFrameCount));
        
        return true;
    }
    catch (const std::exception& e) {
        Utils::Logger::Error("Exception in InitializeAudioOutput: " + std::string(e.what()));
        return false;
    }
}

void AudioMixer::CleanupAudioOutput() {
    Utils::Logger::Info("Cleaning up WASAPI audio output...");
    
    if (m_audioClient) {
        IAudioClient* audioClient = static_cast<IAudioClient*>(m_audioClient);
        audioClient->Stop();
        audioClient->Release();
        m_audioClient = nullptr;
    }
    
    if (m_renderClient) {
        IAudioRenderClient* renderClient = static_cast<IAudioRenderClient*>(m_renderClient);
        renderClient->Release();
        m_renderClient = nullptr;
    }
    
    Utils::Logger::Info("WASAPI audio output cleaned up");
}

void AudioMixer::NotifyMixingStateChanged() {
    if (m_mixingCallback) {
        m_mixingCallback(m_mixingState);
    }
}

void AudioMixer::NotifyError(const std::string& error) {
    if (m_errorCallback) {
        m_errorCallback(error);
    }
}

void AudioMixer::SetMixingCallback(MixingCallback callback) {
    m_mixingCallback = callback;
}

void AudioMixer::SetErrorCallback(ErrorCallback callback) {
    m_errorCallback = callback;
}

} // namespace BluetoothAudio 
