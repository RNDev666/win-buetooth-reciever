#include "Application.h"
#include "Platform/WindowsAPI.h"
#include "Utils/Logger.h"
#include "Utils/StringHelpers.h"
#include <imgui.h>

// Window class name
static const wchar_t* WINDOW_CLASS_NAME = L"BluetoothAudioReceiverWindow";

// Global application instance for window procedure
static Application* g_pApplication = nullptr;

Application::Application() {
    g_pApplication = this;
    
    // Initialize managers
    m_bluetoothSinkService = std::make_unique<BluetoothAudio::BluetoothSinkService>();
    m_audioManager = std::make_unique<BluetoothAudio::AudioManager>();
    m_audioMixer = std::make_shared<BluetoothAudio::AudioMixer>();
    m_configManager = std::make_unique<BluetoothAudio::ConfigManager>();
    m_imguiApp = std::make_unique<BluetoothAudio::ImGuiApp>();
    
    // Connect audio mixer to sink service
    m_bluetoothSinkService->SetAudioMixer(m_audioMixer);
}

Application::~Application() {
    Shutdown();
    g_pApplication = nullptr;
}

bool Application::Initialize() {
    Utils::Logger::Info("Initializing Application...");
    
    // Initialize Windows API
    if (!Platform::WindowsAPI::Initialize()) {
        Utils::Logger::Error("Failed to initialize Windows API");
        return false;
    }
    
    // Initialize configuration manager first
    if (!m_configManager->Initialize()) {
        Utils::Logger::Error("Failed to initialize Config Manager");
        return false;
    }
    
    // Load configuration
    const auto& config = m_configManager->GetConfig();
    
    // Initialize audio manager
    if (!m_audioManager->Initialize()) {
        Utils::Logger::Error("Failed to initialize Audio Manager");
        return false;
    }
    
    // Initialize audio mixer
    if (!m_audioMixer->Initialize()) {
        Utils::Logger::Error("Failed to initialize Audio Mixer");
        return false;
    }
    
    // Initialize bluetooth sink service
    if (!m_bluetoothSinkService->Initialize()) {
        Utils::Logger::Error("Failed to initialize Bluetooth Sink Service");
        return false;
    }
    
    // Set up callbacks
    SetupCallbacks();
    
    // Create main window
    if (!CreateMainWindow()) {
        Utils::Logger::Error("Failed to create main window");
        return false;
    }
    
    // Initialize ImGui
    if (!m_imguiApp->Initialize(m_hwnd)) {
        Utils::Logger::Error("Failed to initialize ImGui");
        return false;
    }
    
    // Create system tray icon
    CreateSystemTrayIcon();
    
    // Show window based on configuration
    if (config.startMinimized) {
        HideMainWindow();
    } else {
        ShowMainWindow();
    }
    
    m_initialized = true;
    Utils::Logger::Info("Application initialized successfully");
    return true;
}

void Application::Run() {
    if (!m_initialized) {
        Utils::Logger::Error("Application not initialized");
        return;
    }
    
    Utils::Logger::Info("Starting application main loop...");
    
    m_running = true;
    
    // Main message loop
    while (m_running) {
        ProcessMessages();
        
        if (m_windowVisible) {
            // Begin ImGui frame
            m_imguiApp->BeginFrame();
            
            // Render UI
            RenderUI();
            
            // End ImGui frame
            m_imguiApp->EndFrame();
            m_imguiApp->Render();
        }
        
        // Small delay to prevent 100% CPU usage
        Sleep(1);
    }
    
    Utils::Logger::Info("Application main loop ended");
}

void Application::Shutdown() {
    if (!m_initialized) return;
    
    Utils::Logger::Info("Shutting down Application...");
    
    m_running = false;
    
    // Destroy system tray icon
    DestroySystemTrayIcon();
    
    // Shutdown ImGui
    m_imguiApp->Shutdown();
    
    // Shutdown managers
    m_bluetoothSinkService->Shutdown();
    m_audioMixer->Shutdown();
    m_audioManager->Shutdown();
    m_configManager->Shutdown();
    
    // Destroy window
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    // Unregister window class
    UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(nullptr));
    
    // Shutdown Windows API
    Platform::WindowsAPI::Cleanup();
    
    m_initialized = false;
    Utils::Logger::Info("Application shutdown complete");
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_pApplication) {
        return g_pApplication->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT Application::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Let ImGui handle the message first
    if (m_imguiApp && m_imguiApp->HandleWindowMessage(hwnd, msg, wParam, lParam)) {
        return 0;
    }
    
    switch (msg) {
        case WM_CLOSE:
            OnClose();
            return 0;
            
        case WM_DESTROY:
            OnDestroy();
            return 0;
            
        case WM_SIZE:
            OnResize(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case WM_SYSCOMMAND:
            if (wParam == SC_MINIMIZE) {
                if (m_configManager->GetConfig().minimizeToTray) {
                    HideMainWindow();
                    return 0;
                }
            }
            break;
            
        case WM_USER + 1: // System tray message
            OnSystemTrayMessage(wParam, lParam);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Private methods
bool Application::CreateMainWindow() {
    RegisterWindowClass();
    
    const auto& config = m_configManager->GetConfig();
    
    // Calculate window position
    int x = config.windowPosX >= 0 ? config.windowPosX : CW_USEDEFAULT;
    int y = config.windowPosY >= 0 ? config.windowPosY : CW_USEDEFAULT;
    int width = config.windowWidth;
    int height = config.windowHeight;
    
    // Create window
    m_hwnd = CreateWindowEx(
        0,
        WINDOW_CLASS_NAME,
        L"Bluetooth Audio Receiver",
        WS_OVERLAPPEDWINDOW,
        x, y, width, height,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!m_hwnd) {
        Utils::Logger::Error("Failed to create main window");
        return false;
    }
    
    Utils::Logger::Info("Main window created successfully");
    return true;
}

void Application::RegisterWindowClass() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
}

void Application::ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_running = false;
            break;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Application::OnSystemTrayMessage(WPARAM wParam, LPARAM lParam) {
    if (lParam == WM_LBUTTONDBLCLK) {
        ToggleMainWindow();
    } else if (lParam == WM_RBUTTONUP) {
        ShowSystemTrayMenu();
    }
}

void Application::ShowMainWindow() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        m_windowVisible = true;
    }
}

void Application::HideMainWindow() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
        m_windowVisible = false;
    }
}

void Application::ToggleMainWindow() {
    if (m_windowVisible) {
        HideMainWindow();
    } else {
        ShowMainWindow();
    }
}

void Application::OnClose() {
    const auto& config = m_configManager->GetConfig();
    
    if (config.minimizeToTray) {
        HideMainWindow();
    } else {
        m_running = false;
    }
}

void Application::OnDestroy() {
    PostQuitMessage(0);
    m_running = false;
}

void Application::OnResize(int width, int height) {
    // Save window size to config
    m_configManager->SetWindowSettings(width, height, -1, -1);
}

void Application::CreateSystemTrayIcon() {
    if (!Platform::WindowsAPI::CreateSystemTrayIcon(m_hwnd, WM_USER + 1)) {
        Utils::Logger::Warn("Failed to create system tray icon");
    }
}

void Application::DestroySystemTrayIcon() {
    Platform::WindowsAPI::DestroySystemTrayIcon();
}

void Application::ShowSystemTrayMenu() {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    
    // Add menu items
    AppendMenu(hMenu, MF_STRING, 1, L"Show/Hide");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, 2, L"Exit");
    
    // Show menu
    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, m_hwnd, nullptr);
    
    // Handle menu selection
    switch (cmd) {
        case 1:
            ToggleMainWindow();
            break;
        case 2:
            m_running = false;
            break;
    }
    
    DestroyMenu(hMenu);
}

void Application::SetupCallbacks() {
    // Bluetooth sink service callbacks
    m_bluetoothSinkService->SetSourceConnectedCallback([this](const Models::ConnectedAudioSource& source) {
        Utils::Logger::Info("Audio source connected: " + source.name);
        if (m_configManager->GetConfig().showNotifications) {
            Platform::WindowsAPI::ShowNotification(L"Source Connected", 
                Utils::StringHelpers::ToWideString("Audio source connected: " + source.name).c_str());
        }
    });
    
    m_bluetoothSinkService->SetSourceDisconnectedCallback([this](const Models::ConnectedAudioSource& source) {
        Utils::Logger::Info("Audio source disconnected: " + source.name);
        if (m_configManager->GetConfig().showNotifications) {
            Platform::WindowsAPI::ShowNotification(L"Source Disconnected", 
                Utils::StringHelpers::ToWideString("Audio source disconnected: " + source.name).c_str());
        }
    });
    
    m_bluetoothSinkService->SetSourceUpdatedCallback([this](const Models::ConnectedAudioSource& source) {
        if (source.IsActive() && m_configManager->GetConfig().showNotifications) {
            Utils::Logger::Debug("Audio source updated: " + source.name + " - " + source.GetStateString());
        }
    });
    
    // Audio mixer callbacks
    m_audioMixer->SetMixingCallback([this](const Models::MixingState& state) {
        Utils::Logger::Debug("Audio mixing state updated - Active sources: " + std::to_string(state.activeSourceCount));
    });
    
    m_audioMixer->SetErrorCallback([this](const std::string& error) {
        Utils::Logger::Error("Audio mixer error: " + error);
        if (m_configManager->GetConfig().showNotifications) {
            Platform::WindowsAPI::ShowNotification(L"Audio Mixer Error", 
                Utils::StringHelpers::ToWideString(error).c_str());
        }
    });
    
    // Audio manager callbacks
    m_audioManager->SetDeviceUpdatedCallback([this](const Models::AudioDevice& device) {
        Utils::Logger::Debug("Audio device updated: " + device.name);
    });
    
    // Config manager callbacks
    m_configManager->SetConfigChangedCallback([this](const Models::AppConfig& config) {
        Utils::Logger::Debug("Configuration changed");
        // Apply configuration changes
        if (config.enableLogging) {
            Utils::Logger::SetLevel(config.logLevel == "debug" ? Utils::Logger::Level::Debug : 
                                   config.logLevel == "warn" ? Utils::Logger::Level::Warn :
                                   config.logLevel == "error" ? Utils::Logger::Level::Error :
                                   Utils::Logger::Level::Info);
        }
    });
}

void Application::RenderUI() {
    // Create main window without docking
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("Main Window", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    
    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                m_running = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Connected Sources")) {
                m_showConnectedSources = true;
            }
            if (ImGui::MenuItem("Audio Mixing")) {
                m_showAudioMixing = true;
            }
            if (ImGui::MenuItem("Audio Devices")) {
                m_showAudioDevices = true;
            }
            if (ImGui::MenuItem("Settings")) {
                m_showSettings = true;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                m_showAbout = true;
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
    
    ImGui::End();
    
    // Render individual windows
    RenderConnectedSourcesWindow();
    RenderAudioMixingWindow();
    RenderAudioDevicesWindow();
    RenderSettingsWindow();
    RenderAboutWindow();
}

void Application::RenderConnectedSourcesWindow() {
    if (!m_showConnectedSources) return;
    
    ImGui::Begin("Connected Audio Sources", &m_showConnectedSources);
    
    // Service status
    ImGui::Text("Service Status: %s", m_bluetoothSinkService->GetServiceStatus().c_str());
    ImGui::Text("Device Name: %s", m_bluetoothSinkService->GetDeviceName().c_str());
    
    // Service controls
    ImGui::Separator();
    if (m_bluetoothSinkService->IsServiceRunning()) {
        if (ImGui::Button("Stop A2DP Sink Service")) {
            m_bluetoothSinkService->StopSinkService();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "RUNNING");
    } else {
        if (ImGui::Button("Start A2DP Sink Service")) {
            m_bluetoothSinkService->StartSinkService();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "STOPPED");
    }
    
    ImGui::Separator();
    
    // Connected sources list
    const auto& sources = m_bluetoothSinkService->GetConnectedSources();
    
    if (sources.empty()) {
        if (m_bluetoothSinkService->IsServiceRunning()) {
            ImGui::Text("No devices connected.");
            ImGui::Text("Your PC is discoverable as: %s", m_bluetoothSinkService->GetDeviceName().c_str());
            ImGui::Text("Connect from your phone's Bluetooth settings.");
        } else {
            ImGui::Text("A2DP Sink Service is stopped.");
            ImGui::Text("Start the service to allow devices to connect.");
        }
    } else {
        ImGui::Text("Connected Sources (%zu):", sources.size());
        ImGui::Separator();
        
        for (const auto& source : sources) {
            ImGui::PushID(source.id.c_str());
            
            // Source info with colored status
            ImVec4 stateColor(source.GetStateColor(0), source.GetStateColor(1), 
                             source.GetStateColor(2), source.GetStateColor(3));
            
            ImGui::TextColored(stateColor, "●");
            ImGui::SameLine();
            ImGui::Text("%s", source.name.c_str());
            
            ImGui::Text("  Status: %s", source.GetStateString());
            if (source.IsActive()) {
                ImGui::Text("  Codec: %s | Quality: %s", source.GetCodecString(), source.GetQualityString());
                ImGui::Text("  Duration: %s", source.GetConnectionDuration().c_str());
            }
            
            // Volume control
            if (source.state == Models::AudioSourceState::Connected || source.IsActive()) {
                ImGui::Text("  Volume: %s", source.GetVolumeString().c_str());
                
                float volume = source.volume;
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::SliderFloat(("##volume_" + source.id).c_str(), &volume, 0.0f, 1.0f, "%.0f%%")) {
                    m_bluetoothSinkService->SetSourceVolume(source.id, volume);
                }
                
                ImGui::SameLine();
                bool muted = source.isMuted;
                if (ImGui::Checkbox(("Mute##" + source.id).c_str(), &muted)) {
                    m_bluetoothSinkService->SetSourceMute(source.id, muted);
                }
            }
            
            // Action buttons
            if (source.CanDisconnect()) {
                if (ImGui::Button(("Disconnect##" + source.id).c_str())) {
                    m_bluetoothSinkService->DisconnectSource(source.id);
                }
            }
            
            ImGui::Separator();
            ImGui::PopID();
        }
    }
    
    // Statistics
    if (m_bluetoothSinkService->HasConnectedSources()) {
        ImGui::Separator();
        ImGui::Text("Statistics:");
        ImGui::Text("  Connected: %zu | Streaming: %zu", 
                   m_bluetoothSinkService->GetConnectedSourceCount(),
                   m_bluetoothSinkService->GetStreamingSourceCount());
    }
    
    ImGui::End();
}

void Application::RenderAudioDevicesWindow() {
    if (!m_showAudioDevices) return;
    
    ImGui::Begin("Audio Devices", &m_showAudioDevices);
    
    // Status
    ImGui::Text("Status: %s", m_audioManager->GetStatus().c_str());
    ImGui::Separator();
    
    // Device list
    const auto& devices = m_audioManager->GetDevices();
    
    if (devices.empty()) {
        ImGui::Text("No audio devices found.");
    } else {
        for (const auto& device : devices) {
            ImGui::PushID(device.id.c_str());
            
            // Device info
            ImGui::Text("%s %s", device.GetTypeIcon(), device.name.c_str());
            if (device.isDefault) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "(Default)");
            }
            
            // Volume control
            float volume = device.volume;
            if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.2f")) {
                m_audioManager->SetDeviceVolume(device.id, volume);
            }
            
            // Mute button
            bool muted = device.isMuted;
            if (ImGui::Checkbox("Muted", &muted)) {
                m_audioManager->SetDeviceMute(device.id, muted);
            }
            
            // Set as default button
            ImGui::SameLine();
            if (!device.isDefault && ImGui::Button("Set as Default")) {
                m_audioManager->SetDefaultDevice(device.id);
            }
            
            ImGui::Separator();
            ImGui::PopID();
        }
    }
    
    if (ImGui::Button("Refresh")) {
        m_audioManager->RefreshDevices();
    }
    
    ImGui::End();
}

void Application::RenderSettingsWindow() {
    if (!m_showSettings) return;
    
    ImGui::Begin("Settings", &m_showSettings);
    
    auto config = m_configManager->GetConfig();
    bool changed = false;
    
    // General settings
    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ImGui::Checkbox("Minimize to tray", &config.minimizeToTray);
        changed |= ImGui::Checkbox("Start minimized", &config.startMinimized);
        changed |= ImGui::Checkbox("Show notifications", &config.showNotifications);
        changed |= ImGui::Checkbox("Auto start with Windows", &config.autoStart);
    }
    
    // Connection settings
    if (ImGui::CollapsingHeader("Connection")) {
        changed |= ImGui::Checkbox("Auto-connect to last device", &config.autoConnectLast);
        changed |= ImGui::Checkbox("Mute audio on disconnect", &config.muteOnDisconnect);
        
        int timeout = config.connectionTimeout / 1000;
        if (ImGui::SliderInt("Connection timeout (seconds)", &timeout, 5, 60)) {
            config.connectionTimeout = timeout * 1000;
            changed = true;
        }
    }
    
    // Audio settings
    if (ImGui::CollapsingHeader("Audio")) {
        changed |= ImGui::SliderFloat("Default volume", &config.defaultVolume, 0.0f, 1.0f, "%.2f");
    }
    
    // Logging settings
    if (ImGui::CollapsingHeader("Logging")) {
        changed |= ImGui::Checkbox("Enable logging", &config.enableLogging);
        
        const char* logLevels[] = { "trace", "debug", "info", "warn", "error" };
        int currentLevel = 2; // Default to "info"
        for (int i = 0; i < 5; i++) {
            if (config.logLevel == logLevels[i]) {
                currentLevel = i;
                break;
            }
        }
        
        if (ImGui::Combo("Log level", &currentLevel, logLevels, 5)) {
            config.logLevel = logLevels[currentLevel];
            changed = true;
        }
    }
    
    if (changed) {
        m_configManager->SetConfig(config);
    }
    
    ImGui::End();
}

void Application::RenderAboutWindow() {
    if (!m_showAbout) return;
    
    ImGui::Begin("About", &m_showAbout);
    
    ImGui::Text("Bluetooth Audio Receiver");
    ImGui::Text("Version: 1.0.0");
    ImGui::Separator();
    
    ImGui::Text("A modern Windows application that turns your PC into a");
    ImGui::Text("Bluetooth audio receiver using A2DP Sink capabilities.");
    ImGui::Separator();
    
    ImGui::Text("Built with:");
    ImGui::BulletText("Modern C++20");
    ImGui::BulletText("Dear ImGui");
    ImGui::BulletText("Windows 10/11 APIs");
    ImGui::BulletText("DirectX 11");
    
    ImGui::End();
}

void Application::RenderAudioMixingWindow() {
    if (!m_showAudioMixing) return;
    
    ImGui::Begin("Audio Mixing", &m_showAudioMixing);
    
    // Get current mixing state
    const auto& mixingState = m_audioMixer->GetMixingState();
    
    // Mixing status
    ImGui::Text("Mixing Status: %s", mixingState.status.c_str());
    ImGui::Text("Active Sources: %d", mixingState.activeSourceCount);
    ImGui::Text("Latency: %s", mixingState.GetLatencyString().c_str());
    ImGui::Text("CPU Usage: %s", mixingState.GetCpuUsageString().c_str());
    
    ImGui::Separator();
    
    // Mixing controls
    ImGui::Text("Mixing Controls:");
    
    // Start/Stop mixing
    if (m_audioMixer->IsRunning()) {
        if (ImGui::Button("Stop Mixing")) {
            m_audioMixer->StopMixing();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "RUNNING");
    } else {
        if (ImGui::Button("Start Mixing")) {
            m_audioMixer->StartMixing();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "STOPPED");
    }
    
    // Master volume
    float masterVolume = mixingState.config.masterVolume;
    ImGui::Text("Master Volume:");
    ImGui::SetNextItemWidth(300.0f);
    if (ImGui::SliderFloat("##master_volume", &masterVolume, 0.0f, 1.0f, "%.0f%%")) {
        m_audioMixer->SetMasterVolume(masterVolume);
    }
    
    // Mixing mode
    ImGui::Text("Mixing Mode: %s", mixingState.GetMixingModeString());
    
    // PC Audio control
    ImGui::Separator();
    ImGui::Text("PC Audio:");
    float pcVolume = mixingState.config.pcAudioVolume;
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::SliderFloat("PC Volume", &pcVolume, 0.0f, 1.0f, "%.0f%%")) {
        // TODO: Update PC audio volume in mixer
        Utils::Logger::Debug("PC volume changed to: " + std::to_string(pcVolume));
    }
    
    ImGui::SameLine();
    bool mutePCAudio = mixingState.config.mutePCAudio;
    if (ImGui::Checkbox("Mute PC", &mutePCAudio)) {
        m_audioMixer->SetPCAudioEnabled(!mutePCAudio);
    }
    
    // Audio sources section
    if (mixingState.activeSourceCount > 0) {
        ImGui::Separator();
        ImGui::Text("Audio Sources:");
        
        // Show level meters for each active source
        for (const auto& [sourceId, settings] : mixingState.config.sourceSettings) {
            if (!settings.isEnabled) continue;
            
            ImGui::PushID(sourceId.c_str());
            
            // Source name and level
            float level = mixingState.GetSourceLevel(sourceId);
            ImGui::Text("%s:", sourceId.c_str());
            
            // Level meter (visual representation)
            ImGui::SameLine();
            ImGui::ProgressBar(level, ImVec2(100, 0), "");
            
            // Volume control
            float sourceVolume = settings.volume;
            ImGui::SetNextItemWidth(150.0f);
            if (ImGui::SliderFloat(("##src_vol_" + sourceId).c_str(), &sourceVolume, 0.0f, 1.0f, "%.0f%%")) {
                m_audioMixer->SetSourceVolume(sourceId, sourceVolume);
            }
            
            // Mute button
            ImGui::SameLine();
            bool sourceMuted = settings.isMuted;
            if (ImGui::Checkbox(("Mute##" + sourceId).c_str(), &sourceMuted)) {
                m_audioMixer->SetSourceMute(sourceId, sourceMuted);
            }
            
            ImGui::PopID();
        }
    }
    
    // Configuration section
    ImGui::Separator();
    ImGui::Text("Configuration:");
    ImGui::Text("Output Device: %s", mixingState.currentOutputDevice.empty() ? "Default" : mixingState.currentOutputDevice.c_str());
    ImGui::Text("Sample Rate: %d Hz", mixingState.config.outputSampleRate);
    ImGui::Text("Channels: %d", mixingState.config.outputChannels);
    ImGui::Text("Buffer Size: %d samples", mixingState.config.bufferSize);
    
    // Error display
    if (!mixingState.lastError.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Last Error:");
        ImGui::TextWrapped("%s", mixingState.lastError.c_str());
    }
    
    ImGui::End();
} 
