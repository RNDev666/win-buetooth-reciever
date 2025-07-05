#pragma once
#include <string>

namespace Models {

struct AudioDevice {
    std::string id;
    std::string name;
    std::string description;
    bool isDefault = false;
    bool isEnabled = true;
    float volume = 1.0f;
    bool isMuted = false;
    
    // Device properties
    int channels = 2;
    int sampleRate = 44100;
    int bitDepth = 16;
    
    const char* GetTypeIcon() const {
        if (name.find("Speaker") != std::string::npos || 
            name.find("Headphone") != std::string::npos) {
            return "\xef\x80\xa5"; // Speaker icon
        } else if (name.find("USB") != std::string::npos) {
            return "\xef\x8a\x87"; // USB icon
        } else if (name.find("Bluetooth") != std::string::npos) {
            return "\xef\x8a\x94"; // Bluetooth icon
        }
        return "\xef\x80\xa5"; // Default speaker icon
    }
    
    std::string GetVolumeString() const {
        if (isMuted) return "Muted";
        return std::to_string(static_cast<int>(volume * 100)) + "%";
    }
};

} // namespace Models 
