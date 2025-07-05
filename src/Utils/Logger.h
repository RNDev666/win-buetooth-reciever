#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>
#include <windows.h>

namespace Utils {

class Logger {
public:
    enum class Level {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5
    };
    
    static bool Initialize(const std::string& logPath, Level level = Level::Info);
    static void Shutdown();
    static void SetLevel(Level level);
    
    // Simple string-based logging methods
    static void Trace(const std::string& message);
    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warn(const std::string& message);
    static void Error(const std::string& message);
    static void Critical(const std::string& message);
    
    // Convenience methods for Windows errors
    static void LogWindowsError(const std::string& operation, DWORD errorCode = GetLastError());
    static void LogHResult(const std::string& operation, long hr);
    
private:
    static std::shared_ptr<spdlog::logger> s_logger;
    static Level StringToLevel(const std::string& levelStr);
    static spdlog::level::level_enum ToSpdlogLevel(Level level);
};

} // namespace Utils 
