#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Models {

struct AppConfig {
    // UI Settings
    bool minimizeToTray = true;
    bool startMinimized = false;
    bool showNotifications = true;
    bool autoStart = false;
    
    // Connection Settings
    bool autoConnectLast = true;
    std::string lastConnectedDeviceId;
    std::vector<std::string> favoriteDevices;
    
    // Audio Settings
    std::string preferredOutputDevice;
    float defaultVolume = 0.8f;
    bool muteOnDisconnect = false;
    
    // Advanced Settings
    int connectionTimeout = 10000; // ms
    bool enableLogging = true;
    std::string logLevel = "info";
    
    // Window Settings
    int windowWidth = 800;
    int windowHeight = 600;
    int windowPosX = -1;
    int windowPosY = -1;
    bool rememberWindowPosition = true;
    
    // Theme Settings
    int uiScale = 100; // percentage
    bool darkTheme = true;
    
    // Hotkey Settings
    bool enableHotkeys = true;
    std::string toggleConnectionHotkey = "Ctrl+Shift+B";
    std::string showWindowHotkey = "Ctrl+Shift+A";
};

// JSON serialization
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppConfig,
    minimizeToTray, startMinimized, showNotifications, autoStart,
    autoConnectLast, lastConnectedDeviceId, favoriteDevices,
    preferredOutputDevice, defaultVolume, muteOnDisconnect,
    connectionTimeout, enableLogging, logLevel,
    windowWidth, windowHeight, windowPosX, windowPosY, rememberWindowPosition,
    uiScale, darkTheme,
    enableHotkeys, toggleConnectionHotkey, showWindowHotkey
)

} // namespace Models 
