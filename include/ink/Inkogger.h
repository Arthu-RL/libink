#ifndef INKOGGER_H
#define INKOGGER_H

#include <array>
#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

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
    constexpr LevelMetadata(std::string_view _color, std::string_view _desc) :
        color(_color), desc(_desc) {}

    std::string_view color;
    std::string_view desc;
};

// ANSI color codes for terminal output
struct LoggerColors {
    static constexpr std::string_view RESET     = "\033[0m";
    static constexpr std::string_view BLACK     = "\033[30m";
    static constexpr std::string_view RED       = "\033[31m";
    static constexpr std::string_view GREEN     = "\033[32m";
    static constexpr std::string_view YELLOW    = "\033[33m";
    static constexpr std::string_view BLUE      = "\033[34m";
    static constexpr std::string_view MAGENTA   = "\033[35m";
    static constexpr std::string_view CYAN      = "\033[36m";
    static constexpr std::string_view DARK_GRAY = "\033[90;1m";
    static constexpr std::string_view WHITE     = "\033[37m";
    static constexpr std::string_view BOLD_RED  = "\033[31;1m";
    static constexpr std::string_view BOLD      = "\033[1m";
    static constexpr std::string_view UNDERLINE = "\033[4m";
};

static constexpr std::array<LevelMetadata, std::to_underlying(LogLevel::COUNT)> MAP_COLORS_FOR_LEVEL = {{
    LevelMetadata("", "OFF"),
    LevelMetadata(LoggerColors::BOLD_RED, "FATAL"),
    LevelMetadata(LoggerColors::RED, "ERROR"),
    LevelMetadata(LoggerColors::YELLOW, "WARN"),
    LevelMetadata(LoggerColors::GREEN, "INFO"),
    LevelMetadata(LoggerColors::BLUE, "DEBUG"),
    LevelMetadata(LoggerColors::DARK_GRAY, "VERBOSE"),
    LevelMetadata(LoggerColors::CYAN, "TRACE")
}};

// Abstract base interface
class INK_API IInkogger {
public:
    virtual ~IInkogger() = default;

    virtual void setName(const std::string& name) = 0;
    virtual const std::string& getName() const = 0;
    virtual void setLevel(LogLevel level) = 0;
    virtual LogLevel getLevel() const = 0;
    virtual bool isEnabled(LogLevel level) const = 0;
    virtual void log(LogLevel level, std::string_view message, const char* file = nullptr, u32 line = 0) = 0;
    virtual void setLogToFile(const std::string& filepath) = 0;
    virtual void setUseColors(bool useColors) = 0;
};

class INK_API Inkogger : public IInkogger {
public:
    Inkogger(const std::string& name);
    virtual ~Inkogger();

    void setName(const std::string& name) override;
    const std::string& getName() const override { return m_Name; }
    void setLevel(LogLevel level) override;
    LogLevel getLevel() const override { return m_Level.load(std::memory_order_relaxed); }
    bool isEnabled(LogLevel level) const override;
    void log(LogLevel level, std::string_view message, const char* file = nullptr, u32 line = 0) override;

    std::string_view getColorForLevel(LogLevel level) const;
    std::string_view getLevelString(LogLevel level) const;
    void setLogToFile(const std::string& filepath) override;
    void setUseColors(bool useColors) override;

protected:
    void appendCurrentTimestamp(std::string& buffer) const;
    std::string_view extractFilename(const char* path) const;

    std::string m_Name;
    std::atomic<LogLevel> m_Level;
    std::atomic<bool> m_UseColors;
    std::atomic<bool> m_LogToFile;

    std::mutex m_Mutex;
    std::ofstream m_FileStream;

    // Thread-local buffer to eliminate memory allocations during formatting
    static thread_local std::string t_MessageBuffer;
};

// Stream-style logging class
class INK_API LogStream {
public:
    LogStream(std::shared_ptr<IInkogger> logger, LogLevel level, const char* file = nullptr, u32 line = 0)
        : m_Logger(std::move(logger)), m_Level(level), m_File(file), m_Line(line) {}

    ~LogStream() {
        if (m_Logger) {
            // Extract string_view and pass it down
            m_Logger->log(m_Level, m_Stream.view(), m_File, m_Line);
        }
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        m_Stream << value;
        return *this;
    }

private:
    std::shared_ptr<IInkogger> m_Logger;
    LogLevel m_Level;
    std::stringstream m_Stream;
    const char* m_File;
    u32 m_Line;
};

// Voidify idiom to prevent dangling elses in macros
struct LogVoidify {
    void operator&(const LogStream&) {}
};

// Global logger manager
class INK_API LogManager {
public:
    static LogManager& getInstance() {
        static LogManager instance;
        return instance;
    }

    std::shared_ptr<IInkogger> getLogger(const std::string& name);
    std::shared_ptr<IInkogger> getCoreLogger(); // Highly cached fast access

    void setGlobalLevel(LogLevel level);
    void setLogToFile(const std::string& filepath);
    void setUseColors(bool useColors);

private:
    LogManager();
    ~LogManager() = default;

    std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IInkogger>> m_Loggers;
    std::shared_ptr<IInkogger> m_CoreLoggerCache;

    LogLevel m_GlobalLevel = LogLevel::INFO;
    std::string m_GlobalFilePath;
    bool m_GlobalUseColors = true;
};

} // namespace ink

#define INK_CORE_LOGGER ink::LogManager::getInstance().getCoreLogger()

#ifdef INK_DISABLE_LOGGING
#define INK_TRACE ((void)0)
// ... disable all
#else
#define INK_LOG !(INK_CORE_LOGGER)->isEnabled(ink::LogLevel::OFF) ? (void)0 : ink::LogVoidify() & ink::LogStream(INK_CORE_LOGGER, ink::LogLevel::OFF)

#define INKL_TRACE(logger) !(logger)->isEnabled(ink::LogLevel::TRACE) ? (void)0 : ink::LogVoidify() & ink::LogStream((logger), ink::LogLevel::TRACE, __FILE__, __LINE__)
#define INKL_VERBOSE(logger) !(logger)->isEnabled(ink::LogLevel::VERBOSE) ? (void)0 : ink::LogVoidify() & ink::LogStream((logger), ink::LogLevel::VERBOSE, __FILE__, __LINE__)
#define INKL_DEBUG(logger) !(logger)->isEnabled(ink::LogLevel::DEBUG) ? (void)0 : ink::LogVoidify() & ink::LogStream((logger), ink::LogLevel::DEBUG, __FILE__, __LINE__)
#define INKL_INFO(logger) !(logger)->isEnabled(ink::LogLevel::INFO) ? (void)0 : ink::LogVoidify() & ink::LogStream((logger), ink::LogLevel::INFO, __FILE__, __LINE__)
#define INKL_WARN(logger) !(logger)->isEnabled(ink::LogLevel::WARN) ? (void)0 : ink::LogVoidify() & ink::LogStream((logger), ink::LogLevel::WARN, __FILE__, __LINE__)
#define INKL_ERROR(logger) !(logger)->isEnabled(ink::LogLevel::ERROR) ? (void)0 : ink::LogVoidify() & ink::LogStream((logger), ink::LogLevel::ERROR, __FILE__, __LINE__)
#define INKL_FATAL(logger) !(logger)->isEnabled(ink::LogLevel::FATAL) ? (void)0 : ink::LogVoidify() & ink::LogStream((logger), ink::LogLevel::FATAL, __FILE__, __LINE__)

// Core logger bindings
#define INK_TRACE INKL_TRACE(INK_CORE_LOGGER)
#define INK_VERBOSE INKL_VERBOSE(INK_CORE_LOGGER)
#define INK_DEBUG INKL_DEBUG(INK_CORE_LOGGER)
#define INK_INFO  INKL_INFO(INK_CORE_LOGGER)
#define INK_WARN  INKL_WARN(INK_CORE_LOGGER)
#define INK_ERROR INKL_ERROR(INK_CORE_LOGGER)
#define INK_FATAL INKL_FATAL(INK_CORE_LOGGER)
#endif

#endif // INKOGGER_H