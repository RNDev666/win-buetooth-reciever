#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <windows.h>

namespace Utils {

class StringHelpers {
public:
    // String conversion utilities
    static std::wstring ToWideString(const std::string& str);
    static std::string ToUtf8String(const std::wstring& wstr);
    
    // Windows string conversion functions removed - not needed for simple implementation
    
    // Simple string formatting helper
    template<typename... Args>
    static std::string Format(const std::string& format, Args... args) {
        // Simple implementation - just concatenate for now
        std::ostringstream oss;
        oss << format;
        ((oss << " " << args), ...);
        return oss.str();
    }
    
    // Trim whitespace from both ends
    static std::string Trim(const std::string& str);
    static std::string TrimLeft(const std::string& str);
    static std::string TrimRight(const std::string& str);
    
    // Case conversion
    static std::string ToLower(const std::string& str);
    static std::string ToUpper(const std::string& str);
    
    // String replacement
    static std::string Replace(const std::string& str, const std::string& from, const std::string& to);
    static std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);
    
    // String splitting
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    static std::vector<std::string> Split(const std::string& str, const std::string& delimiter);
    
    // String joining
    static std::string Join(const std::vector<std::string>& strings, const std::string& delimiter);
    
    // String checking
    static bool StartsWith(const std::string& str, const std::string& prefix);
    static bool EndsWith(const std::string& str, const std::string& suffix);
    static bool Contains(const std::string& str, const std::string& substring);
    
    // Numeric conversion helpers
    static std::string ToString(int value);
    static std::string ToString(float value, int precision = 2);
    static std::string ToString(double value, int precision = 2);
    static std::string ToString(bool value);
    
    // Path helpers
    static std::string GetFileName(const std::string& path);
    static std::string GetFileExtension(const std::string& path);
    static std::string GetDirectoryPath(const std::string& path);
    
    // Validation - removed for simple implementation
};

} // namespace Utils 
