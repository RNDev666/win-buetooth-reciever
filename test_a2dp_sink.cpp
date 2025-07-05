#include "src/Core/BluetoothSinkService.h"
#include "src/Core/AudioMixer.h"
#include "src/Core/AudioManager.h"
#include "src/Utils/Logger.h"
#include "src/Platform/WindowsAPI.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "=== Bluetooth A2DP Sink Test Application ===" << std::endl;
    std::cout << std::endl;
    
    // Initialize logging
    if (!Utils::Logger::Initialize("test_a2dp_sink.log")) {
        std::cout << "Failed to initialize logger" << std::endl;
        return 1;
    }
    
    Utils::Logger::Info("=== Starting Bluetooth A2DP Sink Test ===");
    
    // Initialize Windows API
    if (!Platform::WindowsAPI::Initialize()) {
        std::cout << "Failed to initialize Windows API" << std::endl;
        Utils::Logger::Error("Failed to initialize Windows API");
        return 1;
    }
    
    std::cout << "✓ Windows API initialized" << std::endl;
    
    // Create and initialize audio mixer
    auto audioMixer = std::make_shared<BluetoothAudio::AudioMixer>();
    if (!audioMixer->Initialize()) {
        std::cout << "Failed to initialize Audio Mixer" << std::endl;
        Utils::Logger::Error("Failed to initialize Audio Mixer");
        return 1;
    }
    
    std::cout << "✓ Audio Mixer initialized" << std::endl;
    
    // Start audio mixing
    if (!audioMixer->StartMixing()) {
        std::cout << "Failed to start audio mixing" << std::endl;
        Utils::Logger::Error("Failed to start audio mixing");
        return 1;
    }
    
    std::cout << "✓ Audio mixing started" << std::endl;
    
    // Create and initialize Bluetooth sink service
    auto bluetoothSink = std::make_unique<BluetoothAudio::BluetoothSinkService>();
    bluetoothSink->SetAudioMixer(audioMixer);
    
    if (!bluetoothSink->Initialize()) {
        std::cout << "Failed to initialize Bluetooth Sink Service" << std::endl;
        Utils::Logger::Error("Failed to initialize Bluetooth Sink Service");
        return 1;
    }
    
    std::cout << "✓ Bluetooth Sink Service initialized" << std::endl;
    
    // Set up callbacks
    bluetoothSink->SetSourceConnectedCallback([](const Models::ConnectedAudioSource& source) {
        std::cout << "🔗 Audio source connected: " << source.name << " (" << source.id << ")" << std::endl;
        std::cout << "   State: " << source.GetStateString() << std::endl;
        std::cout << "   Codec: " << source.GetCodecString() << std::endl;
    });
    
    bluetoothSink->SetSourceDisconnectedCallback([](const Models::ConnectedAudioSource& source) {
        std::cout << "🔌 Audio source disconnected: " << source.name << " (" << source.id << ")" << std::endl;
    });
    
    bluetoothSink->SetSourceUpdatedCallback([](const Models::ConnectedAudioSource& source) {
        if (source.IsActive()) {
            std::cout << "🎵 Audio streaming from: " << source.name << " - " << source.GetVolumeString() << std::endl;
        }
    });
    
    bluetoothSink->SetStatusCallback([](const std::string& status) {
        std::cout << "📊 Status: " << status << std::endl;
    });
    
    // Start the A2DP sink service
    std::cout << std::endl;
    std::cout << "Starting Bluetooth A2DP Sink Service..." << std::endl;
    
    if (!bluetoothSink->StartSinkService()) {
        std::cout << "Failed to start Bluetooth A2DP Sink Service" << std::endl;
        Utils::Logger::Error("Failed to start Bluetooth A2DP Sink Service");
        return 1;
    }
    
    std::cout << "✓ Bluetooth A2DP Sink Service started successfully!" << std::endl;
    std::cout << std::endl;
    std::cout << "🎯 Your PC is now discoverable as: " << bluetoothSink->GetDeviceName() << std::endl;
    std::cout << "📱 Connect your phone or other Bluetooth audio device to start streaming!" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  s - Show status" << std::endl;
    std::cout << "  c - Show connected sources" << std::endl;
    std::cout << "  m - Show mixing state" << std::endl;
    std::cout << "  q - Quit" << std::endl;
    std::cout << std::endl;
    
    // Main loop
    char command;
    bool running = true;
    
    while (running) {
        std::cout << "Enter command (s/c/m/q): ";
        std::cin >> command;
        
        switch (command) {
            case 's':
            case 'S':
                std::cout << std::endl;
                std::cout << "=== STATUS ===" << std::endl;
                std::cout << "Service Running: " << (bluetoothSink->IsServiceRunning() ? "Yes" : "No") << std::endl;
                std::cout << "Device Name: " << bluetoothSink->GetDeviceName() << std::endl;
                std::cout << "Connected Sources: " << bluetoothSink->GetConnectedSourceCount() << std::endl;
                std::cout << "Streaming Sources: " << bluetoothSink->GetStreamingSourceCount() << std::endl;
                std::cout << "Status: " << bluetoothSink->GetServiceStatus() << std::endl;
                std::cout << std::endl;
                break;
                
            case 'c':
            case 'C': {
                std::cout << std::endl;
                std::cout << "=== CONNECTED SOURCES ===" << std::endl;
                const auto& sources = bluetoothSink->GetConnectedSources();
                if (sources.empty()) {
                    std::cout << "No connected sources" << std::endl;
                } else {
                    for (const auto& source : sources) {
                        std::cout << "📱 " << source.name << " (" << source.id << ")" << std::endl;
                        std::cout << "   State: " << source.GetStateString() << std::endl;
                        std::cout << "   Volume: " << source.GetVolumeString() << std::endl;
                        std::cout << "   Codec: " << source.GetCodecString() << std::endl;
                        std::cout << "   Duration: " << source.GetConnectionDuration() << std::endl;
                        std::cout << "   Quality: " << source.GetQualityString() << std::endl;
                        std::cout << std::endl;
                    }
                }
                std::cout << std::endl;
                break;
            }
                
            case 'm':
            case 'M': {
                std::cout << std::endl;
                std::cout << "=== MIXING STATE ===" << std::endl;
                const auto& mixingState = audioMixer->GetMixingState();
                std::cout << "Mixing Enabled: " << (mixingState.isEnabled ? "Yes" : "No") << std::endl;
                std::cout << "Processing: " << (mixingState.isProcessing ? "Yes" : "No") << std::endl;
                std::cout << "Mode: " << mixingState.GetMixingModeString() << std::endl;
                std::cout << "Master Volume: " << static_cast<int>(mixingState.config.masterVolume * 100) << "%" << std::endl;
                std::cout << "Active Sources: " << mixingState.activeSourceCount << std::endl;
                std::cout << "Latency: " << mixingState.GetLatencyString() << std::endl;
                std::cout << "CPU Usage: " << mixingState.GetCpuUsageString() << std::endl;
                std::cout << "Status: " << mixingState.status << std::endl;
                std::cout << std::endl;
                break;
            }
                
            case 'q':
            case 'Q':
                running = false;
                break;
                
            default:
                std::cout << "Invalid command. Use s/c/m/q" << std::endl;
                break;
        }
        
        // Small delay to prevent busy loop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    std::cout << std::endl;
    std::cout << "Shutting down..." << std::endl;
    
    bluetoothSink->StopSinkService();
    bluetoothSink->Shutdown();
    
    audioMixer->StopMixing();
    audioMixer->Shutdown();
    
    Platform::WindowsAPI::Cleanup();
    
    std::cout << "✓ Shutdown complete" << std::endl;
    
    Utils::Logger::Info("=== Bluetooth A2DP Sink Test Complete ===");
    Utils::Logger::Shutdown();
    
    return 0;
}