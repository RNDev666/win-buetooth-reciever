#pragma once
#include "../Models/AppConfig.h"
#include <string>
#include <functional>
#include <mutex>

namespace BluetoothAudio {

class ConfigManager {
public:
    using ConfigChangedCallback = std::function<void(const Models::AppConfig&)>;
    
    ConfigManager();
    ~ConfigManager();
    
    bool Initialize();
    void Shutdown();
    
    // Configuration management
    bool LoadConfig();
    bool SaveConfig();
    bool SaveConfigAsync();
    
    // Configuration access
    const Models::AppConfig& GetConfig() const;
    void SetConfig(const Models::AppConfig& config);
    void UpdateConfig(std::function<void(Models::AppConfig&)> updateFunc);
    
    // Specific settings
    void SetMinimizeToTray(bool enabled);
    void SetStartMinimized(bool enabled);
    void SetShowNotifications(bool enabled);
    void SetAutoStart(bool enabled);
    void SetAutoConnectLast(bool enabled);
    void SetLastConnectedDevice(const std::string& deviceId);
    void SetPreferredOutputDevice(const std::string& deviceId);
    void SetDefaultVolume(float volume);
    void SetMuteOnDisconnect(bool enabled);
    void SetWindowSettings(int width, int height, int posX, int posY);
    void SetLogLevel(const std::string& level);
    
    // Callbacks
    void SetConfigChangedCallback(ConfigChangedCallback callback);
    
    // Status
    bool IsInitialized() const { return m_initialized; }
    std::string GetConfigPath() const;
    
private:
    void NotifyConfigChanged();
    std::string GetDefaultConfigPath() const;
    bool CreateConfigDirectory();
    
    // Member variables
    bool m_initialized = false;
    std::string m_configPath;
    Models::AppConfig m_config;
    ConfigChangedCallback m_configChangedCallback;
    
    mutable std::mutex m_configMutex;
};

} // namespace BluetoothAudio 
