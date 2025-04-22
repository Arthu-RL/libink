#ifndef INKOGGER_H
#define INKOGGER_H

#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <mutex>
#include <array>
#include <unordered_map>
#include <memory>

#include "ink/ink_base.hpp"

#if defined(INK_CONFIG_RELEASE)
#define INK_DISABLE_DEBUG_LOGGING
#endif
#if defined(INK_CONFIG_DIST)
#define INK_DISABLE_LOGGING
#endif

namespace ink {

enum class LogLevel {
    OFF = 0,
    FATAL = 1,
    ERROR = 2,
    WARN = 3,
    INFO = 4,
    DEBUG = 5,
    VERBOSE = 6,
    TRACE = 7,
    COUNT
};

struct LevelMetadata {
    constexpr LevelMetadata(const char* _color, const char* _desc) :
        color(_color), desc(_desc)
    {
        // Empty
    }
    const char* color;
    const char* desc;
};

// ANSI color codes for terminal output
struct LoggerColors {
    static constexpr const char* RESET   = "\033[0m";
    static constexpr const char* BLACK   = "\033[30m";
    static constexpr const char* RED     = "\033[31m";
    static constexpr const char* GREEN   = "\033[32m";
    static constexpr const char* YELLOW  = "\033[33m";
    static constexpr const char* BLUE    = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN    = "\033[36m";
    static constexpr const char* DARK_GRAY = "\033[90;1m";
    static constexpr const char* WHITE   = "\033[37m";
    static constexpr const char* BOLD_RED    = "\033[31;1m";
    static constexpr const char* BOLD    = "\033[1m";
    static constexpr const char* UNDERLINE = "\033[4m";
};

static constexpr std::array<LevelMetadata, static_cast<size_t>(LogLevel::COUNT)> MAP_COLORS_FOR_LEVEL = {{
    LevelMetadata("", "OFF"),
    LevelMetadata(LoggerColors::BOLD_RED, "FATAL"),
    LevelMetadata(LoggerColors::RED, "ERROR"),
    LevelMetadata(LoggerColors::YELLOW, "WARN"),
    LevelMetadata(LoggerColors::GREEN, "INFO"),
    LevelMetadata(LoggerColors::BLUE, "DEBUG"),
    LevelMetadata(LoggerColors::DARK_GRAY, "VERBOSE"),
    LevelMetadata(LoggerColors::CYAN, "TRACE")
}};

class INK_API Inkogger {
public:
    struct LogMessage {
        LogLevel level;
        std::string message;
        std::string timestamp;
        std::string file;
        ink_u32 line;
    };

    Inkogger(const std::string& name);
    ~Inkogger();

    void setName(const std::string& name);
    void setLevel(LogLevel level);
    ink_bool isEnabled(LogLevel level) const;
    void log(LogLevel level, const std::string& message, const char* file, ink_u32 line);
    void log(LogLevel level, const std::string& message);
    std::string getColorForLevel(LogLevel level) const;
    std::string getLevelString(LogLevel level) const;
    void setLogToFile(const std::string& filepath);
    void setUseColors(bool useColors);

private:
    std::string m_Name;
    LogLevel m_Level;
    ink_bool m_UseColors;
    std::mutex m_Mutex;
    std::ofstream m_FileStream;
    ink_bool m_LogToFile;
    std::string getCurrentTimestamp() const;
};

// Stream-style logging class
class INK_API LogStream {
public:
    LogStream(std::shared_ptr<Inkogger> logger, LogLevel level, const char* file, ink_u32 line)
        : m_Logger(logger), m_Level(level), m_File(file), m_Line(line) {}
    LogStream(std::shared_ptr<Inkogger> logger, LogLevel level)
        : m_Logger(logger), m_Level(level) {}

    ~LogStream() {
        if (m_Level != LogLevel::OFF)
            m_Logger->log(m_Level, m_Stream.str(), m_File, m_Line);
        else
            m_Logger->log(m_Level, m_Stream.str());
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        m_Stream << value;
        return *this;
    }

private:
    std::shared_ptr<Inkogger> m_Logger;
    LogLevel m_Level;
    std::stringstream m_Stream;
    const char* m_File;
    ink_u32 m_Line;
};

// Global logger manager
class LogManager {
public:
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }

    std::shared_ptr<Inkogger> getLogger(const std::string& name);
    void setGlobalLevel(LogLevel level);
    void setLogToFile(const std::string& filepath);
    void setUseColors(ink_bool useColors);

private:
    LogManager() = default;
    ~LogManager() = default;
    std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<Inkogger>> m_Loggers;
    LogLevel m_GlobalLevel = LogLevel::INFO;
    std::string m_GlobalFilePath;
    ink_bool m_GlobalUseColors = true;
};

}

// Define a core logger for global access
#define INK_CORE_LOGGER ink::LogManager::getInstance().getLogger("INK")

// Convenience macros that use the core logger
#ifdef INK_DISABLE_LOGGING
#define INK_TRACE ((void)0)
#define INK_DEBUG ((void)0)
#define INK_INFO  ((void)0)
#define INK_WARN  ((void)0)
#define INK_ERROR ((void)0)
#define INK_FATAL ((void)0)
#else
#define INK_LOG ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::OFF)
#define INK_TRACE ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::TRACE, __FILE__, __LINE__)
#define INK_VERBOSE ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::VERBOSE, __FILE__, __LINE__)
#define INK_DEBUG ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::DEBUG, __FILE__, __LINE__)
#define INK_INFO  ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::INFO, __FILE__, __LINE__)
#define INK_WARN  ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::WARN, __FILE__, __LINE__)
#define INK_ERROR ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::ERROR, __FILE__, __LINE__)
#define INK_FATAL ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::FATAL, __FILE__, __LINE__)
#endif

#endif // INKOGGER_H

