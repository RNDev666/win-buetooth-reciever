#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace Models {

enum class MixingMode {
    Disabled,       // No mixing - only PC audio
    Replace,        // Bluetooth audio replaces PC audio
    Mix,            // Mix Bluetooth audio with PC audio
    Crossfade       // Crossfade between sources
};

struct ChannelMapping {
    int sourceChannel = 0;
    int targetChannel = 0;
    float gain = 1.0f;
};

struct AudioSourceMixSettings {
    std::string sourceId;
    float volume = 1.0f;            // 0.0-1.0
    float gain = 0.0f;              // dB
    bool isMuted = false;
    bool isEnabled = true;
    
    // Advanced mixing settings
    float pan = 0.0f;               // -1.0 (left) to 1.0 (right)
    float delay = 0.0f;             // ms
    bool enableEQ = false;
    std::vector<float> eqBands;     // EQ band gains in dB
    
    // Channel mapping for surround sound
    std::vector<ChannelMapping> channelMap;
};

struct MixingConfiguration {
    MixingMode mode = MixingMode::Mix;
    float masterVolume = 1.0f;
    bool enableMasterLimiter = true;
    float limiterThreshold = -3.0f;     // dB
    
    // PC audio settings
    float pcAudioVolume = 1.0f;
    bool mutePCAudio = false;
    
    // Buffer settings
    int bufferSize = 1024;              // samples
    int latencyTarget = 50;             // ms
    
    // Quality settings
    int outputSampleRate = 48000;       // Hz
    int outputBitDepth = 16;            // bits
    int outputChannels = 2;             // stereo
    
    // Individual source settings
    std::unordered_map<std::string, AudioSourceMixSettings> sourceSettings;
};

struct MixingStats {
    float currentLatency = 0.0f;        // ms
    float cpuUsage = 0.0f;              // 0.0-1.0
    int bufferUnderruns = 0;
    int bufferOverruns = 0;
    
    // Per-source stats
    std::unordered_map<std::string, float> sourceLevels;    // Current audio levels
    std::unordered_map<std::string, int> sourceDropouts;   // Dropout counts
};

struct MixingState {
    MixingConfiguration config;
    MixingStats stats;
    
    bool isEnabled = false;
    bool isProcessing = false;
    int activeSourceCount = 0;
    std::string currentOutputDevice;
    
    // Status information
    std::string status = "Stopped";
    std::string lastError;
    
    const char* GetMixingModeString() const {
        switch (config.mode) {
            case MixingMode::Disabled: return "Disabled";
            case MixingMode::Replace: return "Replace PC Audio";
            case MixingMode::Mix: return "Mix with PC Audio";
            case MixingMode::Crossfade: return "Crossfade";
            default: return "Unknown";
        }
    }
    
    std::string GetLatencyString() const {
        return std::to_string(static_cast<int>(stats.currentLatency)) + " ms";
    }
    
    std::string GetCpuUsageString() const {
        return std::to_string(static_cast<int>(stats.cpuUsage * 100)) + "%";
    }
    
    // Helper methods for source management
    AudioSourceMixSettings& GetOrCreateSourceSettings(const std::string& sourceId) {
        auto it = config.sourceSettings.find(sourceId);
        if (it == config.sourceSettings.end()) {
            AudioSourceMixSettings newSettings;
            newSettings.sourceId = sourceId;
            config.sourceSettings[sourceId] = newSettings;
            return config.sourceSettings[sourceId];
        }
        return it->second;
    }
    
    void RemoveSourceSettings(const std::string& sourceId) {
        config.sourceSettings.erase(sourceId);
        stats.sourceLevels.erase(sourceId);
        stats.sourceDropouts.erase(sourceId);
    }
    
    float GetSourceLevel(const std::string& sourceId) const {
        auto it = stats.sourceLevels.find(sourceId);
        return it != stats.sourceLevels.end() ? it->second : 0.0f;
    }
    
    void UpdateSourceLevel(const std::string& sourceId, float level) {
        stats.sourceLevels[sourceId] = level;
    }
};

} // namespace Models 
