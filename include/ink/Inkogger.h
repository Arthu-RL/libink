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
#include <atomic>

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

// Abstract base interface for all loggers
class INK_API IInkogger {
public:
    struct LogMessage {
        LogLevel level;
        std::string message;
        std::string timestamp;
        std::string file;
        u32 line;
    };

    virtual ~IInkogger() = default;

    virtual void setName(const std::string& name) = 0;
    virtual std::string getName() const = 0;
    virtual void setLevel(LogLevel level) = 0;
    virtual LogLevel getLevel() const = 0;
    virtual bool isEnabled(LogLevel level) const = 0;
    virtual void log(LogLevel level, const std::string& message, const char* file = nullptr, u32 line = 0) = 0;
    virtual void setLogToFile(const std::string& filepath) = 0;
    virtual void setUseColors(bool useColors) = 0;
};

class INK_API Inkogger : public IInkogger {
public:
    Inkogger(const std::string& name);
    virtual ~Inkogger();

    void setName(const std::string& name) override;
    std::string getName() const override { return m_Name; }
    void setLevel(LogLevel level) override;
    LogLevel getLevel() const override { return m_Level; }
    bool isEnabled(LogLevel level) const override;
    void log(LogLevel level, const std::string& message, const char* file = nullptr, u32 line = 0) override;
    std::string getColorForLevel(LogLevel level) const;
    std::string getLevelString(LogLevel level) const;
    void setLogToFile(const std::string& filepath) override;
    void setUseColors(bool useColors) override;

protected:
    std::string getCurrentTimestamp() const;
    std::string extractFilename(const char* path) const;

    std::string m_Name;
    std::atomic<LogLevel> m_Level;
    std::atomic<bool> m_UseColors;
    std::mutex m_Mutex;
    std::ofstream m_FileStream;
    std::atomic<bool> m_LogToFile;

    // Thread-local buffer for message formatting
    static thread_local std::string t_MessageBuffer;
};

// Stream-style logging class
class INK_API LogStream {
public:
    LogStream(std::shared_ptr<IInkogger> logger, LogLevel level, const char* file = nullptr, u32 line = 0)
        : m_Logger(logger), m_Level(level), m_File(file), m_Line(line) {}

    ~LogStream() {
        if (m_Logger) {
            m_Logger->log(m_Level, m_Stream.str(), m_File, m_Line);
        }
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        if (m_Logger && m_Logger->isEnabled(m_Level)) {
            m_Stream << value;
        }
        return *this;
    }

private:
    std::shared_ptr<IInkogger> m_Logger;
    LogLevel m_Level;
    std::stringstream m_Stream;
    const char* m_File;
    u32 m_Line;
};

// Global logger manager
class LogManager {
public:
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }

    std::shared_ptr<IInkogger> getLogger(const std::string& name);
    std::shared_ptr<IInkogger> registerLogger(const std::string& name, std::shared_ptr<IInkogger> logger);
    void setGlobalLevel(LogLevel level);
    void setLogToFile(const std::string& filepath);
    void setUseColors(bool useColors);

private:
    LogManager();
    ~LogManager() = default;
    std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IInkogger>> m_Loggers;
    LogLevel m_GlobalLevel = LogLevel::INFO;
    std::string m_GlobalFilePath;
    bool m_GlobalUseColors = true;
};

}

// Define a core logger for global access
#define INK_CORE_LOGGER ink::LogManager::getInstance().getLogger("INK")

// Convenience macros that use the core logger
#ifdef INK_DISABLE_LOGGING
#define INK_TRACE ((void)0)
#define INK_VERBOSE ((void)0)
#define INK_DEBUG ((void)0)
#define INK_INFO  ((void)0)
#define INK_WARN  ((void)0)
#define INK_ERROR ((void)0)
#define INK_FATAL ((void)0)
#define INK_LOG ((void)0)
#else
#define INK_LOG ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::OFF)
#define INK_TRACE ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::TRACE, __FILE__, __LINE__)
#define INK_VERBOSE ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::VERBOSE, __FILE__, __LINE__)
#define INK_DEBUG ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::DEBUG, __FILE__, __LINE__)
#define INK_INFO ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::INFO, __FILE__, __LINE__)
#define INK_WARN ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::WARN, __FILE__, __LINE__)
#define INK_ERROR ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::ERROR, __FILE__, __LINE__)
#define INK_FATAL ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::FATAL, __FILE__, __LINE__)

#define INKL_TRACE(logger) ink::LogStream(logger, ink::LogLevel::TRACE, __FILE__, __LINE__)
#define INKL_VERBOSE(logger) ink::LogStream(logger, ink::LogLevel::VERBOSE, __FILE__, __LINE__)
#define INKL_DEBUG(logger) ink::LogStream(logger, ink::LogLevel::DEBUG, __FILE__, __LINE__)
#define INKL_INFO(logger) ink::LogStream(logger, ink::LogLevel::INFO, __FILE__, __LINE__)
#define INKL_WARN(logger) ink::LogStream(logger, ink::LogLevel::WARN, __FILE__, __LINE__)
#define INKL_ERROR(logger) ink::LogStream(logger, ink::LogLevel::ERROR, __FILE__, __LINE__)
#define INKL_FATAL(logger) ink::LogStream(logger, ink::LogLevel::FATAL, __FILE__, __LINE__)

#endif

#endif // INKOGGER_H
