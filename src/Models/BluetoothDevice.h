#pragma once
#include <string>
#include <chrono>

namespace Models {

enum class ConnectionState {
    Disconnected,
    Enabling,
    Enabled,
    Connecting,
    Connected,
    Error
};

struct BluetoothDevice {
    std::string id;
    std::string name;
    std::string containerId;
    ConnectionState state = ConnectionState::Disconnected;
    bool isEnabled = false;
    bool isConnected = false;
    std::chrono::system_clock::time_point lastSeen;
    std::string errorMessage;
    
    // UI state
    bool isSelected = false;
    
    const char* GetStateString() const {
        switch (state) {
            case ConnectionState::Disconnected: return "Disconnected";
            case ConnectionState::Enabling: return "Enabling...";
            case ConnectionState::Enabled: return "Enabled";
            case ConnectionState::Connecting: return "Connecting...";
            case ConnectionState::Connected: return "Connected";
            case ConnectionState::Error: return "Error";
            default: return "Unknown";
        }
    }
    
    bool CanEnable() const {
        return state == ConnectionState::Disconnected;
    }
    
    bool CanConnect() const {
        return state == ConnectionState::Enabled && !isConnected;
    }
    
    bool CanDisconnect() const {
        return isConnected;
    }
    
    float GetStateColor(int component) const {
        switch (state) {
            case ConnectionState::Connected:
                return component == 1 ? 0.8f : (component == 3 ? 1.0f : 0.0f); // Green
            case ConnectionState::Enabled:
                return component == 2 ? 0.8f : (component == 3 ? 1.0f : 0.0f); // Blue
            case ConnectionState::Error:
                return component == 0 ? 0.8f : (component == 3 ? 1.0f : 0.0f); // Red
            default:
                return component == 3 ? 1.0f : 0.6f; // Gray
        }
    }
};

} // namespace Models 
