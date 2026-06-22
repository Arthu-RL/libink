#include "../include/ink/Inkogger.h"
#include <format>
#include <chrono>
#include <cstring>
#include <iterator>

namespace ink {

thread_local std::string Inkogger::t_MessageBuffer;

Inkogger::Inkogger(const std::string& name)
    : m_Name(name), m_Level(LogLevel::INFO), m_UseColors(true), m_LogToFile(false)
{
    // Pre-allocate to prevent formatting reallocations
    if (t_MessageBuffer.capacity() < 2048) {
        t_MessageBuffer.reserve(2048);
    }
}

Inkogger::~Inkogger()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_FileStream.is_open())
    {
        m_FileStream.flush();
        m_FileStream.close();
    }
}

void Inkogger::setName(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Name = name;
}

void Inkogger::setLevel(LogLevel level)
{
    m_Level.store(level, std::memory_order_relaxed);
}

bool Inkogger::isEnabled(LogLevel level) const
{
    return level <= m_Level.load(std::memory_order_relaxed);
}

std::string_view Inkogger::getColorForLevel(LogLevel level) const
{
    if (level >= LogLevel::COUNT) return "";
    return MAP_COLORS_FOR_LEVEL[std::to_underlying(level)].color;
}

std::string_view Inkogger::getLevelString(LogLevel level) const
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

std::string_view Inkogger::extractFilename(const char* path) const
{
    if (!path) return "";
    const char* lastSlash = std::strrchr(path, '/');
    const char* lastBackslash = std::strrchr(path, '\\');

    if (lastSlash) return lastSlash + 1;
    if (lastBackslash) return lastBackslash + 1;
    return path;
}

void Inkogger::log(LogLevel level, std::string_view message, const char* file, u32 line)
{
    // Clear thread-local buffer (resets size to 0, keeps memory allocated)
    t_MessageBuffer.clear();

    if (level == LogLevel::OFF)
    {
        t_MessageBuffer.append(message);
        t_MessageBuffer.push_back('\n');

        fwrite(t_MessageBuffer.data(), sizeof(char), t_MessageBuffer.size(), stdout);
        fflush(stdout);

        if (m_LogToFile.load(std::memory_order_relaxed))
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_FileStream.is_open()) {
                m_FileStream.write(t_MessageBuffer.data(), t_MessageBuffer.size());
                m_FileStream.flush();
            }
        }
        return;
    }

    // Double check just in case, though macros handle this
    if (!isEnabled(level)) return;

    bool useColors = m_UseColors.load(std::memory_order_relaxed);
    std::string_view color = useColors ? getColorForLevel(level) : "";
    std::string_view reset = useColors ? LoggerColors::RESET : "";
    std::string_view levelStr = getLevelString(level);

    // formating directly to the thread-local buffer
    t_MessageBuffer.push_back('[');
    appendCurrentTimestamp(t_MessageBuffer);

    std::format_to(std::back_inserter(t_MessageBuffer), "] {}[{}]{} [{}]: {}",
                   color, levelStr, reset, m_Name, message);

    if (file != nullptr) {
        std::format_to(std::back_inserter(t_MessageBuffer), " ({}:{})", extractFilename(file), line);
    }

    t_MessageBuffer.push_back('\n');

    // Print to console
    fwrite(t_MessageBuffer.data(), sizeof(char), t_MessageBuffer.size(), stdout);
    fflush(stdout);

    // Print to file
    if (m_LogToFile.load(std::memory_order_relaxed))
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_FileStream.is_open())
        {
            if (useColors)
            {
                // Clean version without ANSI escapes for file
                std::string fileBuffer;
                fileBuffer.reserve(t_MessageBuffer.size());
                fileBuffer.push_back('[');
                appendCurrentTimestamp(fileBuffer);
                std::format_to(std::back_inserter(fileBuffer), "] [{}] [{}]: {}", levelStr, m_Name, message);

                if (file != nullptr) {
                    std::format_to(std::back_inserter(fileBuffer), " ({}:{})", extractFilename(file), line);
                }
                fileBuffer.push_back('\n');

                m_FileStream.write(fileBuffer.data(), fileBuffer.size());
            }
            else
            {
                m_FileStream.write(t_MessageBuffer.data(), t_MessageBuffer.size());
            }
            m_FileStream.flush();
        }
    }
}

void Inkogger::setLogToFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_FileStream.is_open()) m_FileStream.close();

    m_LogToFile.store(!filepath.empty(), std::memory_order_relaxed);

    if (!filepath.empty()) {
        m_FileStream.open(filepath, std::ios::out | std::ios::app);
        if (!m_FileStream.is_open())
        {
            m_LogToFile.store(false, std::memory_order_relaxed);
            const char* errorMsg = "Failed to open log file: ";
            fwrite(errorMsg, 1, strlen(errorMsg), stderr);
            fwrite(filepath.c_str(), 1, filepath.size(), stderr);
            fwrite("\n", 1, 1, stderr);
        }
    }
}

void Inkogger::setUseColors(bool useColors)
{
    m_UseColors.store(useColors, std::memory_order_relaxed);
}

// LogManager Implementation
LogManager::LogManager()
    : m_GlobalLevel(LogLevel::INFO), m_GlobalUseColors(true) {}

std::shared_ptr<IInkogger> LogManager::getLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (auto it = m_Loggers.find(name); it != m_Loggers.end()) {
        return it->second;
    }

    auto logger = std::make_shared<Inkogger>(name);
    logger->setLevel(m_GlobalLevel);
    if (!m_GlobalFilePath.empty()) logger->setLogToFile(m_GlobalFilePath);
    logger->setUseColors(m_GlobalUseColors);

    m_Loggers[name] = logger;
    return logger;
}

std::shared_ptr<IInkogger> LogManager::getCoreLogger()
{
    // Fast path: cached read without locks. Perfect for the macro.
    if (m_CoreLoggerCache) {
        return m_CoreLoggerCache;
    }

    // Slow path: only runs once on startup
    m_CoreLoggerCache = getLogger("INK");
    return m_CoreLoggerCache;
}

void LogManager::setGlobalLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GlobalLevel = level;
    for (auto& [name, logger] : m_Loggers) {
        logger->setLevel(level);
    }
}

void LogManager::setLogToFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GlobalFilePath = filepath;
    for (auto& [name, logger] : m_Loggers) {
        logger->setLogToFile(filepath);
    }
}

void LogManager::setUseColors(bool useColors)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_GlobalUseColors = useColors;
    for (auto& [name, logger] : m_Loggers) {
        logger->setUseColors(useColors);
    }
}

} // namespace ink