#pragma once
#include <string>
#include <chrono>

namespace Models {

enum class AudioSourceState {
    Disconnected,
    Connecting,
    Connected,
    Streaming,
    Paused,
    Error
};

enum class AudioCodec {
    Unknown,
    SBC,        // Subband Codec
    AAC,        // Advanced Audio Coding
    aptX,       // Qualcomm aptX
    LDAC        // Sony LDAC
};

struct AudioStreamInfo {
    AudioCodec codec = AudioCodec::SBC;
    int sampleRate = 44100;     // Hz
    int bitRate = 328;          // kbps
    int channels = 2;           // Stereo
    float latency = 0.0f;       // ms
    float quality = 0.0f;       // 0.0-1.0
};

struct ConnectedAudioSource {
    std::string id;
    std::string name;
    std::string deviceType;         // Phone, Tablet, Computer, etc.
    std::string macAddress;
    
    // Connection state
    AudioSourceState state = AudioSourceState::Disconnected;
    std::chrono::system_clock::time_point lastActivity;
    std::chrono::system_clock::time_point connectedAt;
    std::string errorMessage;
    
    // Audio properties
    AudioStreamInfo streamInfo;
    float volume = 1.0f;            // 0.0-1.0
    bool isMuted = false;
    bool isStreaming = false;
    
    // Connection quality
    float signalStrength = 0.0f;    // 0.0-1.0
    int packetsLost = 0;
    int packetsReceived = 0;
    
    // UI state
    bool isSelected = false;
    bool showDetails = false;
    
    const char* GetStateString() const {
        switch (state) {
            case AudioSourceState::Disconnected: return "Disconnected";
            case AudioSourceState::Connecting: return "Connecting...";
            case AudioSourceState::Connected: return "Connected";
            case AudioSourceState::Streaming: return "Streaming";
            case AudioSourceState::Paused: return "Paused";
            case AudioSourceState::Error: return "Error";
            default: return "Unknown";
        }
    }
    
    const char* GetCodecString() const {
        switch (streamInfo.codec) {
            case AudioCodec::SBC: return "SBC";
            case AudioCodec::AAC: return "AAC";
            case AudioCodec::aptX: return "aptX";
            case AudioCodec::LDAC: return "LDAC";
            default: return "Unknown";
        }
    }
    
    std::string GetVolumeString() const {
        if (isMuted) return "Muted";
        return std::to_string(static_cast<int>(volume * 100)) + "%";
    }
    
    std::string GetBitrateString() const {
        return std::to_string(streamInfo.bitRate) + " kbps";
    }
    
    std::string GetQualityString() const {
        if (streamInfo.quality < 0.3f) return "Poor";
        if (streamInfo.quality < 0.6f) return "Fair";
        if (streamInfo.quality < 0.8f) return "Good";
        return "Excellent";
    }
    
    bool CanDisconnect() const {
        return state == AudioSourceState::Connected || 
               state == AudioSourceState::Streaming ||
               state == AudioSourceState::Paused;
    }
    
    bool IsActive() const {
        return state == AudioSourceState::Streaming;
    }
    
    float GetStateColor(int component) const {
        switch (state) {
            case AudioSourceState::Streaming:
                return component == 1 ? 0.9f : (component == 3 ? 1.0f : 0.0f); // Bright Green
            case AudioSourceState::Connected:
                return component == 1 ? 0.6f : (component == 3 ? 1.0f : 0.0f); // Green
            case AudioSourceState::Paused:
                return component == 2 ? 0.8f : (component == 3 ? 1.0f : 0.0f); // Blue
            case AudioSourceState::Error:
                return component == 0 ? 0.8f : (component == 3 ? 1.0f : 0.0f); // Red
            default:
                return component == 3 ? 1.0f : 0.5f; // Gray
        }
    }
    
    // Calculate connection duration
    std::string GetConnectionDuration() const {
        if (state == AudioSourceState::Disconnected) return "Not connected";
        
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - connectedAt);
        
        int totalSeconds = static_cast<int>(duration.count());
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        
        if (hours > 0) {
            return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
        } else if (minutes > 0) {
            return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
        } else {
            return std::to_string(seconds) + "s";
        }
    }
};

} // namespace Models 
