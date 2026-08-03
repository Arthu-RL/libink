#include "../include/ink/Inkogger.h"
#include <format>
#include <chrono>
#include <cstring>
#include <iterator>

#if defined(INK_PLATFORM_ANDROID)
#include <android/log.h>
#endif

#if !INK_LOGGER_USE_OFSTREAM
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ink {

namespace {

#if defined(INK_PLATFORM_ANDROID)
// Maps INK's LogLevel to the Android NDK log priority so messages are
// correctly filtered/colored by logcat and routed through logd instead of
// a stdout stream that Android does not attach to logcat by default.
constexpr android_LogPriority toAndroidPriority(LogLevel level) noexcept
{
    switch (level)
    {
        case LogLevel::FATAL:   return ANDROID_LOG_FATAL;
        case LogLevel::ERROR:   return ANDROID_LOG_ERROR;
        case LogLevel::WARN:    return ANDROID_LOG_WARN;
        case LogLevel::INFO:    return ANDROID_LOG_INFO;
        case LogLevel::DEBUG:   return ANDROID_LOG_DEBUG;
        case LogLevel::VERBOSE:
        case LogLevel::TRACE:   return ANDROID_LOG_VERBOSE;
        case LogLevel::OFF:
        default:                return ANDROID_LOG_INFO;
    }
}
#endif

// Only FATAL/ERROR/WARN force an immediate flush; INFO and below rely on
// normal stdio/ofstream buffering so high-frequency logging (VERBOSE/TRACE
// in tight engine loops) doesn't pay a syscall per line.
constexpr bool requiresImmediateFlush(LogLevel level) noexcept
{
    return level <= LogLevel::WARN;
}

} // namespace

thread_local std::string Inkogger::t_MessageBuffer;
thread_local std::string Inkogger::t_PlainMessageBuffer;

Inkogger::Inkogger(const std::string& name)
    : _name(name), _level(LogLevel::INFO), _useColors(true), _logToFile(false)
{
    // Pre-allocate to prevent formatting reallocations
    if (t_MessageBuffer.capacity() < 2048) {
        t_MessageBuffer.reserve(2048);
    }
}

Inkogger::~Inkogger()
{
    closeLogFile();
}

void Inkogger::setName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _name = name;
}

void Inkogger::setLevel(LogLevel level) noexcept
{
    _level.store(level, std::memory_order_relaxed);
}

bool Inkogger::isEnabled(LogLevel level) const noexcept
{
    return level <= _level.load(std::memory_order_relaxed);
}

std::string_view Inkogger::getColorForLevel(LogLevel level) const noexcept
{
    if (level >= LogLevel::COUNT) return "";
    return MAP_COLORS_FOR_LEVEL[std::to_underlying(level)].color;
}

std::string_view Inkogger::getLevelString(LogLevel level) const noexcept
{
    if (level >= LogLevel::COUNT) return "UNKNOWN";
    return MAP_COLORS_FOR_LEVEL[std::to_underlying(level)].desc;
}

void Inkogger::appendCurrentTimestamp(std::string& buffer) const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm timeinfo{};
#ifdef _WIN32
    localtime_s(&timeinfo, &time);
#else
    localtime_r(&time, &timeinfo);
#endif

    // Format directly into the provided buffer (Zero new string allocations)
    std::format_to(std::back_inserter(buffer), "{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
                   timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                   timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, ms.count());
}

std::string_view Inkogger::extractFilename(const char* path) const noexcept
{
    if (!path) return "";
    const char* lastSlash = std::strrchr(path, '/');
    const char* lastBackslash = std::strrchr(path, '\\');

    if (lastSlash) return lastSlash + 1;
    if (lastBackslash) return lastBackslash + 1;
    return path;
}

void Inkogger::writeToPlatformConsole(LogLevel level, std::string_view plainMessage,
                                       std::string_view coloredMessage) const
{
#if defined(INK_PLATFORM_ANDROID)
    INK_UNUSED(coloredMessage);

    // logcat timestamps/tags entries itself and does not render ANSI escapes,
    // so Android always routes the plain (non-colored) form through logd
    // instead of stdout, which is not attached to logcat on Android.
    std::string_view body = plainMessage;
    if (!body.empty() && body.back() == '\n')
        body.remove_suffix(1);

    __android_log_print(toAndroidPriority(level), _name.c_str(), "%.*s",
                         static_cast<int>(body.size()), body.data());
#else
    INK_UNUSED(plainMessage);

    std::fwrite(coloredMessage.data(), sizeof(char), coloredMessage.size(), stdout);
    // Flushing every line serializes on a syscall per log call; only pay that
    // cost for severities where losing buffered output on a crash matters.
    if (requiresImmediateFlush(level))
        std::fflush(stdout);

#endif
}

void Inkogger::log(LogLevel level, std::string_view message, const char* file, u32 line)
{
    // Clear thread-local buffer (resets size to 0, keeps memory allocated)
    t_MessageBuffer.clear();

    if (level == LogLevel::OFF)
    {
        t_MessageBuffer.append(message);
        t_MessageBuffer.push_back('\n');

        writeToPlatformConsole(LogLevel::OFF, t_MessageBuffer, t_MessageBuffer);

        if (_logToFile.load(std::memory_order_relaxed)) {
            writeToFile(t_MessageBuffer, /*flushNow=*/true);
        }
        return;
    }

    // Double check just in case, though macros handle this
    if (!isEnabled(level)) return;

    const bool useColors = _useColors.load(std::memory_order_relaxed);
    const bool logToFile = _logToFile.load(std::memory_order_relaxed);
    std::string_view color = useColors ? getColorForLevel(level) : "";
    std::string_view reset = useColors ? LoggerColors::RESET : "";
    std::string_view levelStr = getLevelString(level);

    // formating directly to the thread-local buffer
    t_MessageBuffer.push_back('[');
    appendCurrentTimestamp(t_MessageBuffer);

    std::format_to(std::back_inserter(t_MessageBuffer), "] {}[{}]{} [{}]: {}",
                   color, levelStr, reset, _name, message);

    if (file != nullptr) {
        std::format_to(std::back_inserter(t_MessageBuffer), " ({}:{})", extractFilename(file), line);
    }

    t_MessageBuffer.push_back('\n');

    // A plain (ANSI-free) copy is only formatted when something actually
    // consumes it: Android's console sink never wants ANSI escapes, and the
    // log file shouldn't be polluted with them either. When colors are off
    // t_MessageBuffer is already plain, so no second pass is needed.
#if defined(INK_PLATFORM_ANDROID)
    const bool needsPlainBuffer = useColors;
#else
    const bool needsPlainBuffer = useColors && logToFile;
#endif

    std::string_view plainView = t_MessageBuffer;
    if (needsPlainBuffer)
    {
        t_PlainMessageBuffer.clear();
        t_PlainMessageBuffer.reserve(t_MessageBuffer.size());
        t_PlainMessageBuffer.push_back('[');
        appendCurrentTimestamp(t_PlainMessageBuffer);
        std::format_to(std::back_inserter(t_PlainMessageBuffer), "] [{}] [{}]: {}", levelStr, _name, message);

        if (file != nullptr) 
        {
            std::format_to(std::back_inserter(t_PlainMessageBuffer), " ({}:{})", extractFilename(file), line);
        }
        t_PlainMessageBuffer.push_back('\n');
        plainView = t_PlainMessageBuffer;
    }

    // Print to console (stdout on desktop/WASM, logd/logcat on Android)
    writeToPlatformConsole(level, plainView, t_MessageBuffer);

    // Print to file
    if (logToFile)
        writeToFile(plainView, requiresImmediateFlush(level));
}

bool Inkogger::openLogFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(_mutex);

#if INK_LOGGER_USE_OFSTREAM
    if (_fileStream.is_open()) _fileStream.close();
    _fileStream.open(filepath, std::ios::out | std::ios::app);
    return _fileStream.is_open();
#else
    const int newFd = ::open(filepath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (newFd < 0) return false;

    const int oldFd = _fileFd.exchange(newFd, std::memory_order_acq_rel);
    if (oldFd >= 0)
        ::close(oldFd);
 
    return true;
#endif
}

void Inkogger::closeLogFile()
{
    std::lock_guard<std::mutex> lock(_mutex);

#if INK_LOGGER_USE_OFSTREAM
    if (_fileStream.is_open()) 
    {
        _fileStream.flush();
        _fileStream.close();
    }
#else
    const int oldFd = _fileFd.exchange(-1, std::memory_order_acq_rel);
    if (oldFd >= 0) 
        ::close(oldFd);

#endif
}

void Inkogger::writeToFile(std::string_view data, bool flushNow) const
{
#if INK_LOGGER_USE_OFSTREAM
    std::lock_guard<std::mutex> lock(_mutex);
    if (_fileStream.is_open()) 
    {
        _fileStream.write(data.data(), static_cast<std::streamsize>(data.size()));
        if (flushNow)
            _fileStream.flush();
    }
#else
    INK_UNUSED(flushNow); // write() has no userspace buffer to flush

    const int fd = _fileFd.load(std::memory_order_acquire);
    if (fd < 0) 
        return;

    std::size_t written = 0;
    while (written < data.size()) 
    {
        const ssize_t n = ::write(fd, data.data() + written, data.size() - written);
        if (n <= 0) 
            break;
        written += static_cast<std::size_t>(n);
    }
#endif
}

void Inkogger::setLogToFile(const std::string& filepath)
{
    if (filepath.empty()) 
    {
        closeLogFile();
        _logToFile.store(false, std::memory_order_relaxed);
        return;
    }

    if (openLogFile(filepath)) 
    {
        _logToFile.store(true, std::memory_order_relaxed);
    } 
    else 
    {
        _logToFile.store(false, std::memory_order_relaxed);
        const char* errorMsg = "Failed to open log file: ";
        fwrite(errorMsg, 1, strlen(errorMsg), stderr);
        fwrite(filepath.c_str(), 1, filepath.size(), stderr);
        fwrite("\n", 1, 1, stderr);
    }
}

void Inkogger::setUseColors(bool useColors) noexcept
{
    _useColors.store(useColors, std::memory_order_relaxed);
}

// LogManager Implementation
LogManager::LogManager()
    : _globalLevel(LogLevel::INFO), _globalUseColors(true) {}

std::shared_ptr<IInkogger> LogManager::getLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (auto it = _loggers.find(name); it != _loggers.end()) {
        return it->second;
    }

    auto logger = std::make_shared<Inkogger>(name);
    logger->setLevel(_globalLevel);
    if (!_globalFilePath.empty()) logger->setLogToFile(_globalFilePath);
    logger->setUseColors(_globalUseColors);

    _loggers[name] = logger;
    return logger;
}

std::shared_ptr<IInkogger> LogManager::getCoreLogger()
{
    // std::call_once synchronizes every caller with the one thread that
    // actually runs the initializer, so the plain (non-atomic) read of
    // _coreLoggerCache below is race-free even on the very first call from
    // multiple threads.
    std::call_once(_coreLoggerOnceFlag, [this]() {
        _coreLoggerCache = getLogger("INK");
    });
    return _coreLoggerCache;
}

void LogManager::setGlobalLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(_mutex);
    _globalLevel = level;
    for (auto& [name, logger] : _loggers) 
    {
        logger->setLevel(level);
    }
}

void LogManager::setLogToFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _globalFilePath = filepath;
    for (auto& [name, logger] : _loggers) 
    {
        logger->setLogToFile(filepath);
    }
}

void LogManager::setUseColors(bool useColors)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _globalUseColors = useColors;
    for (auto& [name, logger] : _loggers) 
    {
        logger->setUseColors(useColors);
    }
}

} // namespace ink