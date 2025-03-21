#ifndef VACLOGGER_H
#define VACLOGGER_H

#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <memory>

#include "vac/vac_base.hpp"

#if defined(VAC_CONFIG_RELEASE)
#define VAC_DISABLE_DEBUG_LOGGING
#endif
#if defined(VAC_CONFIG_DIST)
#define VAC_DISABLE_LOGGING
#endif

namespace vac {

enum class LogLevel {
    OFF = 0,
    FATAL = 1,
    ERROR = 2,
    WARN = 3,
    INFO = 4,
    DEBUG = 5,
    VERBOSE = 6,
    TRACE = 7
};

class Logger {
public:
    struct LogMessage {
        LogLevel level;
        std::string message;
        std::string timestamp;
        std::string file;
        vac_u32 line;
    };

    // ANSI color codes for terminal output
    struct Colors {
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

    Logger(const std::string& name);
    ~Logger();

    void setLevel(LogLevel level);
    vac_bool isEnabled(LogLevel level) const;
    void log(LogLevel level, const std::string& message, const char* file, vac_u32 line);
    std::string getColorForLevel(LogLevel level) const;
    std::string getLevelString(LogLevel level) const;
    void setLogToFile(const std::string& filepath);
    void setUseColors(bool useColors);

private:
    std::string m_Name;
    LogLevel m_Level;
    vac_bool m_UseColors;
    std::mutex m_Mutex;
    std::ofstream m_FileStream;
    vac_bool m_LogToFile;
    std::string getCurrentTimestamp() const;
};

// Stream-style logging class
class LogStream {
public:
    LogStream(std::shared_ptr<Logger> logger, LogLevel level, const char* file, vac_u32 line)
        : m_Logger(logger), m_Level(level), m_File(file), m_Line(line) {}

    ~LogStream() {
        m_Logger->log(m_Level, m_Stream.str(), m_File, m_Line);
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        m_Stream << value;
        return *this;
    }

private:
    std::shared_ptr<Logger> m_Logger;
    LogLevel m_Level;
    std::stringstream m_Stream;
    const char* m_File;
    vac_u32 m_Line;
};

// Global logger manager
class LogManager {
public:
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }

    std::shared_ptr<Logger> getLogger(const std::string& name);
    void setGlobalLevel(LogLevel level);
    void setLogToFile(const std::string& filepath);
    void setUseColors(vac_bool useColors);

private:
    LogManager() = default;
    ~LogManager() = default;
    std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<Logger>> m_Loggers;
    LogLevel m_GlobalLevel = LogLevel::INFO;
    std::string m_GlobalFilePath;
    vac_bool m_GlobalUseColors = true;
};

// Helper macros for the logger
#ifdef VAC_DISABLE_LOGGING
#define VAC_LOG_TRACE(logger) ((void)0)
#define VAC_LOG_DEBUG(logger) ((void)0)
#define VAC_LOG_INFO(logger)  ((void)0)
#define VAC_LOG_WARN(logger)  ((void)0)
#define VAC_LOG_ERROR(logger) ((void)0)
#define VAC_LOG_FATAL(logger) ((void)0)
#else
#define VAC_LOG_TRACE(logger) \
vac::LogStream(logger, vac::LogLevel::TRACE, __FILE__, __LINE__)

#define VAC_LOG_VERBOSE(logger) \
    vac::LogStream(logger, vac::LogLevel::VERBOSE, __FILE__, __LINE__)

#define VAC_LOG_DEBUG(logger) \
    vac::LogStream(logger, vac::LogLevel::DEBUG, __FILE__, __LINE__)

#define VAC_LOG_INFO(logger) \
    vac::LogStream(logger, vac::LogLevel::INFO, __FILE__, __LINE__)

#define VAC_LOG_WARN(logger) \
    vac::LogStream(logger, vac::LogLevel::WARN, __FILE__, __LINE__)

#define VAC_LOG_ERROR(logger) \
    vac::LogStream(logger, vac::LogLevel::ERROR, __FILE__, __LINE__)

#define VAC_LOG_FATAL(logger) \
    vac::LogStream(logger, vac::LogLevel::FATAL, __FILE__, __LINE__)
#endif

}

// Define a core logger for global access
#define VAC_CORE_LOGGER vac::LogManager::getInstance().getLogger("AURA")

// Convenience macros that use the core logger
#ifdef VAC_DISABLE_LOGGING
#define VAC_TRACE ((void)0)
#define VAC_DEBUG ((void)0)
#define VAC_INFO  ((void)0)
#define VAC_WARN  ((void)0)
#define VAC_ERROR ((void)0)
#define VAC_FATAL ((void)0)
#else
#define VAC_TRACE vac::LogStream(VAC_CORE_LOGGER, vac::LogLevel::TRACE, __FILE__, __LINE__)
#define VAC_VERBOSE vac::LogStream(VAC_CORE_LOGGER, vac::LogLevel::VERBOSE, __FILE__, __LINE__)
#define VAC_DEBUG vac::LogStream(VAC_CORE_LOGGER, vac::LogLevel::DEBUG, __FILE__, __LINE__)
#define VAC_INFO  vac::LogStream(VAC_CORE_LOGGER, vac::LogLevel::INFO, __FILE__, __LINE__)
#define VAC_WARN  vac::LogStream(VAC_CORE_LOGGER, vac::LogLevel::WARN, __FILE__, __LINE__)
#define VAC_ERROR vac::LogStream(VAC_CORE_LOGGER, vac::LogLevel::ERROR, __FILE__, __LINE__)
#define VAC_FATAL vac::LogStream(VAC_CORE_LOGGER, vac::LogLevel::FATAL, __FILE__, __LINE__)
#endif

#endif // AURALOGGER_H

