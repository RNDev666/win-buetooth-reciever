#pragma once
#include "Core/BluetoothSinkService.h"
#include "Core/AudioManager.h"
#include "Core/AudioMixer.h"
#include "Core/ConfigManager.h"
#include "UI/ImGuiApp.h"
#include <windows.h>
#include <memory>

class Application {
public:
    Application();
    ~Application();
    
    bool Initialize();
    void Run();
    void Shutdown();
    
    // Window message handling
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
private:
    bool CreateMainWindow();
    void RegisterWindowClass();
    void ProcessMessages();
    void OnSystemTrayMessage(WPARAM wParam, LPARAM lParam);
    void ShowMainWindow();
    void HideMainWindow();
    void ToggleMainWindow();
    void OnClose();
    void OnDestroy();
    void OnResize(int width, int height);
    
    // System tray
    void CreateSystemTrayIcon();
    void DestroySystemTrayIcon();
    void ShowSystemTrayMenu();
    
    // Setup
    void SetupCallbacks();
    
    // UI rendering
    void RenderUI();
    void RenderConnectedSourcesWindow();
    void RenderAudioMixingWindow();
    void RenderAudioDevicesWindow();
    void RenderSettingsWindow();
    void RenderAboutWindow();
    
    // Core managers
    std::unique_ptr<BluetoothAudio::BluetoothSinkService> m_bluetoothSinkService;
    std::unique_ptr<BluetoothAudio::AudioManager> m_audioManager;
    std::shared_ptr<BluetoothAudio::AudioMixer> m_audioMixer;
    std::unique_ptr<BluetoothAudio::ConfigManager> m_configManager;
    
    // UI
    std::unique_ptr<BluetoothAudio::ImGuiApp> m_imguiApp;
    // std::unique_ptr<Platform::SystemTray> m_systemTray; // Not implemented yet
    
    // Window management
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    bool m_initialized = false;
    bool m_running = false;
    bool m_windowVisible = true;
    bool m_minimizeToTray = false;
    
    // UI state
    bool m_showConnectedSources = true;
    bool m_showAudioMixing = true;
    bool m_showAudioDevices = true;
    bool m_showSettings = false;
    bool m_showAbout = false;
    
    // Constants
    static constexpr UINT WM_TRAY_CALLBACK = WM_USER + 1;
    static constexpr LPCWSTR WINDOW_CLASS_NAME = L"BluetoothAudioReceiverWindow";
    static constexpr LPCWSTR WINDOW_TITLE = L"Bluetooth Audio Receiver";
    
    // Singleton for window proc
    static Application* s_instance;
}; 
