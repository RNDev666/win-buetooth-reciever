#include "ConfigManager.h"
#include "../Utils/Logger.h"
#include "../Utils/StringHelpers.h"
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>

namespace BluetoothAudio {

ConfigManager::ConfigManager() = default;

ConfigManager::~ConfigManager() {
    Shutdown();
}

bool ConfigManager::Initialize() {
    if (m_initialized) return true;
    
    Utils::Logger::Info("Initializing Config Manager...");
    
    // Set default config path
    m_configPath = GetDefaultConfigPath();
    
    // Create config directory if it doesn't exist
    if (!CreateConfigDirectory()) {
        Utils::Logger::Error("Failed to create config directory");
        return false;
    }
    
    // Load configuration
    if (!LoadConfig()) {
        Utils::Logger::Warn("Failed to load config, using defaults");
        // Continue with default configuration
    }
    
    m_initialized = true;
    Utils::Logger::Info("Config Manager initialized successfully");
    return true;
}

void ConfigManager::Shutdown() {
    if (!m_initialized) return;
    
    Utils::Logger::Info("Shutting down Config Manager...");
    
    // Save current configuration
    SaveConfig();
    
    m_initialized = false;
    Utils::Logger::Info("Config Manager shut down complete");
}

bool ConfigManager::LoadConfig() {
    try {
        if (!std::filesystem::exists(m_configPath)) {
            Utils::Logger::Info("Config file doesn't exist, using defaults: " + m_configPath);
            return true; // Use default config
        }
        
        std::ifstream file(m_configPath);
        if (!file.is_open()) {
            Utils::Logger::Error("Failed to open config file: " + m_configPath);
            return false;
        }
        
        nlohmann::json jsonConfig;
        file >> jsonConfig;
        
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_config = jsonConfig.get<Models::AppConfig>();
        
        Utils::Logger::Info("Successfully loaded configuration from: " + m_configPath);
        return true;
        
    } catch (const std::exception& e) {
        Utils::Logger::Error("Error loading config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::SaveConfig() {
    try {
        std::lock_guard<std::mutex> lock(m_configMutex);
        
        nlohmann::json jsonConfig = m_config;
        
        std::ofstream file(m_configPath);
        if (!file.is_open()) {
            Utils::Logger::Error("Failed to open config file for writing: " + m_configPath);
            return false;
        }
        
        file << jsonConfig.dump(4); // Pretty print with 4 spaces
        
        Utils::Logger::Debug("Successfully saved configuration to: " + m_configPath);
        return true;
        
    } catch (const std::exception& e) {
        Utils::Logger::Error("Error saving config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::SaveConfigAsync() {
    // For now, just save synchronously
    // In a more advanced implementation, this could queue the save operation
    return SaveConfig();
}

const Models::AppConfig& ConfigManager::GetConfig() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config;
}

void ConfigManager::SetConfig(const Models::AppConfig& config) {
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_config = config;
    }
    
    NotifyConfigChanged();
    SaveConfigAsync();
}

void ConfigManager::UpdateConfig(std::function<void(Models::AppConfig&)> updateFunc) {
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        updateFunc(m_config);
    }
    
    NotifyConfigChanged();
    SaveConfigAsync();
}

void ConfigManager::SetMinimizeToTray(bool enabled) {
    UpdateConfig([enabled](Models::AppConfig& config) {
        config.minimizeToTray = enabled;
    });
}

void ConfigManager::SetStartMinimized(bool enabled) {
    UpdateConfig([enabled](Models::AppConfig& config) {
        config.startMinimized = enabled;
    });
}

void ConfigManager::SetShowNotifications(bool enabled) {
    UpdateConfig([enabled](Models::AppConfig& config) {
        config.showNotifications = enabled;
    });
}

void ConfigManager::SetAutoStart(bool enabled) {
    UpdateConfig([enabled](Models::AppConfig& config) {
        config.autoStart = enabled;
    });
}

void ConfigManager::SetAutoConnectLast(bool enabled) {
    UpdateConfig([enabled](Models::AppConfig& config) {
        config.autoConnectLast = enabled;
    });
}

void ConfigManager::SetLastConnectedDevice(const std::string& deviceId) {
    UpdateConfig([deviceId](Models::AppConfig& config) {
        config.lastConnectedDeviceId = deviceId;
    });
}

void ConfigManager::SetPreferredOutputDevice(const std::string& deviceId) {
    UpdateConfig([deviceId](Models::AppConfig& config) {
        config.preferredOutputDevice = deviceId;
    });
}

void ConfigManager::SetDefaultVolume(float volume) {
    UpdateConfig([volume](Models::AppConfig& config) {
        config.defaultVolume = std::max(0.0f, std::min(1.0f, volume));
    });
}

void ConfigManager::SetMuteOnDisconnect(bool enabled) {
    UpdateConfig([enabled](Models::AppConfig& config) {
        config.muteOnDisconnect = enabled;
    });
}

void ConfigManager::SetWindowSettings(int width, int height, int posX, int posY) {
    UpdateConfig([width, height, posX, posY](Models::AppConfig& config) {
        config.windowWidth = width;
        config.windowHeight = height;
        config.windowPosX = posX;
        config.windowPosY = posY;
    });
}

void ConfigManager::SetLogLevel(const std::string& level) {
    UpdateConfig([level](Models::AppConfig& config) {
        config.logLevel = level;
    });
}

void ConfigManager::SetConfigChangedCallback(ConfigChangedCallback callback) {
    m_configChangedCallback = callback;
}

std::string ConfigManager::GetConfigPath() const {
    return m_configPath;
}

// Private methods
void ConfigManager::NotifyConfigChanged() {
    if (m_configChangedCallback) {
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_configChangedCallback(m_config);
    }
}

std::string ConfigManager::GetDefaultConfigPath() const {
    // Get the AppData/Roaming directory
    PWSTR appDataPath = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath);
    
    if (SUCCEEDED(hr) && appDataPath) {
        std::wstring wAppDataPath(appDataPath);
        CoTaskMemFree(appDataPath);
        
        std::string appDataPathStr = Utils::StringHelpers::ToUtf8String(wAppDataPath);
        std::filesystem::path configPath = std::filesystem::path(appDataPathStr) / "BluetoothAudioReceiver";
        
        return (configPath / "config.json").string();
    }
    
    // Fallback to current directory
    return "config.json";
}

bool ConfigManager::CreateConfigDirectory() {
    try {
        std::filesystem::path configPath(m_configPath);
        std::filesystem::path configDir = configPath.parent_path();
        
        if (!configDir.empty() && !std::filesystem::exists(configDir)) {
            std::filesystem::create_directories(configDir);
            Utils::Logger::Info("Created config directory: " + configDir.string());
        }
        
        return true;
    } catch (const std::exception& e) {
        Utils::Logger::Error("Error creating config directory: " + std::string(e.what()));
        return false;
    }
}

} // namespace BluetoothAudio 
