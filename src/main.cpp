#include "Application.h"
#include "Utils/Logger.h"
#include <windows.h>
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize logging
    if (!Utils::Logger::Initialize("logs/bluetooth_audio_receiver.log")) {
        MessageBox(nullptr, L"Failed to initialize logger", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    Utils::Logger::Info("=== Bluetooth Audio Receiver Starting ===");
    
    // Create and initialize application
    Application app;
    
    if (!app.Initialize()) {
        Utils::Logger::Error("Failed to initialize application");
        MessageBox(nullptr, L"Failed to initialize application. Check logs for details.", L"Error", MB_OK | MB_ICONERROR);
        Utils::Logger::Shutdown();
        return 1;
    }
    
    // Run application
    app.Run();
    
    // Cleanup
    app.Shutdown();
    
    Utils::Logger::Info("=== Bluetooth Audio Receiver Exiting ===");
    Utils::Logger::Shutdown();
    
    return 0;
} 
