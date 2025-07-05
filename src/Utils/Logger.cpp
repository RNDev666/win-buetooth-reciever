#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iostream>
#include <filesystem>
#include <windows.h>

namespace Utils {

std::shared_ptr<spdlog::logger> Logger::s_logger = nullptr;

bool Logger::Initialize(const std::string& logPath, Level level) {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::path logDir = std::filesystem::path(logPath).parent_path();
        if (!logDir.empty() && !std::filesystem::exists(logDir)) {
            std::filesystem::create_directories(logDir);
        }
        
        // Create sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logPath, 1024 * 1024 * 5, 3); // 5MB max file size, 3 files
        
        // Create logger
        s_logger = std::make_shared<spdlog::logger>("BluetoothAudioReceiver",
            spdlog::sinks_init_list{console_sink, file_sink});
        
        // Set level
        s_logger->set_level(ToSpdlogLevel(level));
        s_logger->flush_on(spdlog::level::err);
        
        s_logger->info("Logger initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Logger initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void Logger::Shutdown() {
    if (s_logger) {
        s_logger->info("Logger shutting down");
        s_logger->flush();
        s_logger.reset();
    }
}

void Logger::SetLevel(Level level) {
    if (s_logger) {
        s_logger->set_level(ToSpdlogLevel(level));
    }
}

void Logger::Trace(const std::string& message) {
    if (s_logger) s_logger->trace(message);
}

void Logger::Debug(const std::string& message) {
    if (s_logger) s_logger->debug(message);
}

void Logger::Info(const std::string& message) {
    if (s_logger) s_logger->info(message);
}

void Logger::Warn(const std::string& message) {
    if (s_logger) s_logger->warn(message);
}

void Logger::Error(const std::string& message) {
    if (s_logger) s_logger->error(message);
}

void Logger::Critical(const std::string& message) {
    if (s_logger) s_logger->critical(message);
}

void Logger::LogWindowsError(const std::string& operation, DWORD errorCode) {
    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer, 0, nullptr);
    
    std::string errorMessage = "Unknown error";
    if (messageBuffer) {
        // Convert to UTF-8
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, messageBuffer, -1, nullptr, 0, nullptr, nullptr);
        if (utf8Size > 0) {
            std::string utf8Message(utf8Size - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, messageBuffer, -1, utf8Message.data(), utf8Size, nullptr, nullptr);
            errorMessage = utf8Message;
        }
        LocalFree(messageBuffer);
    }
    
    Error(operation + ": " + errorMessage + " (Code: " + std::to_string(errorCode) + ")");
}

void Logger::LogHResult(const std::string& operation, long hr) {
    Error(operation + ": HRESULT 0x" + std::to_string(hr));
}

Logger::Level Logger::StringToLevel(const std::string& levelStr) {
    if (levelStr == "trace") return Level::Trace;
    if (levelStr == "debug") return Level::Debug;
    if (levelStr == "info") return Level::Info;
    if (levelStr == "warn") return Level::Warn;
    if (levelStr == "error") return Level::Error;
    if (levelStr == "critical") return Level::Critical;
    return Level::Info;
}

spdlog::level::level_enum Logger::ToSpdlogLevel(Level level) {
    switch (level) {
        case Level::Trace: return spdlog::level::trace;
        case Level::Debug: return spdlog::level::debug;
        case Level::Info: return spdlog::level::info;
        case Level::Warn: return spdlog::level::warn;
        case Level::Error: return spdlog::level::err;
        case Level::Critical: return spdlog::level::critical;
        default: return spdlog::level::info;
    }
}

} // namespace Utils 
