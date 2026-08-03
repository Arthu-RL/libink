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

// On POSIX kernels (Linux, Android) a raw fd opened with O_APPEND 
// lets the kernel serialize concurrent writes to a regular 
// file atomically instead, so the per-line write path
// can be lock-free. Windows has no equivalent guarantee without different
// (FILE_APPEND_DATA) plumbing, and Emscripten's virtual filesystems don't
// document the same atomicity across pthread workers, so both fall back to
// the mutex-guarded ofstream path.
#if defined(INK_PLATFORM_WINDOWS) || defined(__EMSCRIPTEN__)
#define INK_LOGGER_USE_OFSTREAM 1
#else
#define INK_LOGGER_USE_OFSTREAM 0
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
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;
    virtual void setLevel(LogLevel level) noexcept = 0;
    [[nodiscard]] virtual LogLevel getLevel() const noexcept = 0;
    [[nodiscard]] virtual bool isEnabled(LogLevel level) const noexcept = 0;
    virtual void log(LogLevel level, std::string_view message, const char* file = nullptr, u32 line = 0) = 0;
    virtual void setLogToFile(const std::string& filepath) = 0;
    virtual void setUseColors(bool useColors) noexcept = 0;
};

class INK_API Inkogger : public IInkogger {
public:
    explicit Inkogger(const std::string& name);
    ~Inkogger() override;

    void setName(const std::string& name) override;
    [[nodiscard]] const std::string& getName() const noexcept override { return _name; }
    void setLevel(LogLevel level) noexcept override;
    [[nodiscard]] LogLevel getLevel() const noexcept override { return _level.load(std::memory_order_relaxed); }
    [[nodiscard]] bool isEnabled(LogLevel level) const noexcept override;
    void log(LogLevel level, std::string_view message, const char* file = nullptr, u32 line = 0) override;

    [[nodiscard]] std::string_view getColorForLevel(LogLevel level) const noexcept;
    [[nodiscard]] std::string_view getLevelString(LogLevel level) const noexcept;
    void setLogToFile(const std::string& filepath) override;
    void setUseColors(bool useColors) noexcept override;

protected:
    void appendCurrentTimestamp(std::string& buffer) const;
    std::string_view extractFilename(const char* path) const noexcept;
    void writeToPlatformConsole(LogLevel level, std::string_view plainMessage,
                                 std::string_view coloredMessage) const;

    // Opens/repoints the log file. Not intended to be called concurrently
    // with active logging from other threads (see the note on _fileFd
    // below
    bool openLogFile(const std::string& filepath);
    void closeLogFile();
    // Writes one already-formatted, newline-terminated line to the log
    // file. flushNow only affects the ofstream fallback path: the fd path
    // has no userspace buffering to flush, every write() is already visible
    // as soon as it returns.
    void writeToFile(std::string_view data, bool flushNow) const;

    std::string _name;
    std::atomic<LogLevel> _level;
    std::atomic<bool> _useColors;
    std::atomic<bool> _logToFile;

    // Guards file lifecycle transitions only (open/close via
    // setLogToFile/openLogFile/closeLogFile, and the destructor). The
    // per-line write path (writeToFile) does not take this lock on POSIX
    // see _fileFd.
    mutable std::mutex _mutex;
#if INK_LOGGER_USE_OFSTREAM
    mutable std::ofstream _fileStream;
#else
    // Raw fd opened O_WRONLY|O_APPEND. The kernel serializes concurrent
    // write() calls to a regular file opened this way, so multiple logging
    // threads can write without taking a lock. This does NOT make
    // reconfiguration (closing/reopening via setLogToFile) safe to run
    // concurrently with in-flight writes: a write() that already loaded the
    // old fd value can race with another thread's close() of that same fd
    // number. setLogToFile() is expected to be called during setup, not
    // interleaved with hot-path logging from other threads.
    std::atomic<int> _fileFd{-1};
#endif

    // Thread-local buffers to eliminate memory allocations during formatting
    static thread_local std::string t_MessageBuffer;
    // Lazily-populated ANSI-free copy, used for the Android console sink and
    // for file output when colors are enabled on the primary buffer.
    static thread_local std::string t_PlainMessageBuffer;
};

// Stream-style logging class
class INK_API LogStream {
public:
    LogStream(std::shared_ptr<IInkogger> logger, LogLevel level, const char* file = nullptr, u32 line = 0)
        : _logger(std::move(logger)), _level(level), _file(file), _line(line) {}

    ~LogStream() {
        if (_logger) {
            // Extract string_view and pass it down
            _logger->log(_level, _stream.view(), _file, _line);
        }
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        _stream << value;
        return *this;
    }

private:
    std::shared_ptr<IInkogger> _logger;
    LogLevel _level;
    std::stringstream _stream;
    const char* _file;
    u32 _line;
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

    std::mutex _mutex;
    std::unordered_map<std::string, std::shared_ptr<IInkogger>> _loggers;
    // std::call_once guarantees the write in getCoreLogger()'s slow path
    // happens-before every fast-path read, without an atomic<shared_ptr>
    // (unsupported by the Android NDK's libc++ for non-trivially-copyable T)
    // and without a per-call mutex lock.
    std::once_flag _coreLoggerOnceFlag;
    std::shared_ptr<IInkogger> _coreLoggerCache;

    LogLevel _globalLevel = LogLevel::INFO;
    std::string _globalFilePath;
    bool _globalUseColors = true;
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