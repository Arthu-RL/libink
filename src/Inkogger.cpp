#include "../include/ink/Inkogger.h"

#include <sstream>
#include <chrono>
#include <iomanip>
#include <cstring>

namespace ink {

thread_local std::string Inkogger::t_MessageBuffer;

Inkogger::Inkogger(const std::string& name)
    : m_Name(name), m_Level(LogLevel::INFO), m_UseColors(true), m_LogToFile(false)
{
    t_MessageBuffer.reserve(1024);
}

Inkogger::~Inkogger()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_FileStream.is_open())
    {
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

std::string Inkogger::getColorForLevel(LogLevel level) const
{
    if (!m_UseColors.load(std::memory_order_relaxed)) return "";

    if (level >= LogLevel::COUNT) {
        return "";
    }

    return MAP_COLORS_FOR_LEVEL[static_cast<size_t>(level)].color;
}

std::string Inkogger::getLevelString(LogLevel level) const
{
    if (level >= LogLevel::COUNT) {
        return "UNKNOWN";
    }

    return MAP_COLORS_FOR_LEVEL[static_cast<size_t>(level)].desc;
}

std::string Inkogger::getCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::stringstream ss;

    // Use thread-safe localtime
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time);
#else
    localtime_r(&time, &timeinfo);
#endif

    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Inkogger::extractFilename(const char* path) const
{
    if (!path) return "";

    const char* filename = path;
    const char* lastSlash = std::strrchr(path, '/');
    const char* lastBackslash = std::strrchr(path, '\\');

    if (lastSlash != nullptr) {
        filename = lastSlash + 1;
    } else if (lastBackslash != nullptr) {
        filename = lastBackslash + 1;
    }

    return filename;
}

void Inkogger::log(LogLevel level, const std::string& message, const char* file, u32 line)
{
    // Special case for direct logging (OFF level)
    if (level == LogLevel::OFF && !message.empty())
    {
        const char* result = message.data();
        // Output to console - use fwrite for better performance than cout
        fwrite(result, 1, message.size(), stdout);
        fwrite("\n", 1, 1, stdout);
        fflush(stdout);

        // Output to file if enabled
        if (m_LogToFile.load(std::memory_order_relaxed) && m_FileStream.is_open())
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_FileStream.is_open()) {
                m_FileStream << result << std::endl;
            }
        }
        return;
    }

    if (!isEnabled(level)) return;

    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = getLevelString(level);
    std::string color = getColorForLevel(level);
    std::string reset = m_UseColors.load(std::memory_order_relaxed) ? LoggerColors::RESET : "";

    t_MessageBuffer.clear();

    // Format: [timestamp] [level] [name]: message (file:line)
    t_MessageBuffer += '[';
    t_MessageBuffer += timestamp;
    t_MessageBuffer += "] ";

    t_MessageBuffer += color;
    t_MessageBuffer += '[';
    t_MessageBuffer += levelStr;
    t_MessageBuffer += ']';
    t_MessageBuffer += reset;

    t_MessageBuffer += " [";
    t_MessageBuffer += m_Name;
    t_MessageBuffer += "]: ";

    t_MessageBuffer += message;

    // Only add file/line if provided
    if (file != nullptr)
    {
        std::string filename = extractFilename(file);

        t_MessageBuffer += " (";
        t_MessageBuffer += filename;
        t_MessageBuffer += ':';
        t_MessageBuffer += std::to_string(line);
        t_MessageBuffer += ')';
    }

    fwrite(t_MessageBuffer.data(), 1, t_MessageBuffer.size(), stdout);
    fwrite("\n", 1, 1, stdout);
    fflush(stdout);

    if (m_LogToFile.load(std::memory_order_relaxed))
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_FileStream.is_open())
        {
            if (m_UseColors.load(std::memory_order_relaxed))
            {
                std::string fileLine;
                fileLine.reserve(t_MessageBuffer.size());

                fileLine += '[';
                fileLine += timestamp;
                fileLine += "] [";
                fileLine += levelStr;
                fileLine += "] [";
                fileLine += m_Name;
                fileLine += "]: ";
                fileLine += message;

                if (file != nullptr)
                {
                    std::string filename = extractFilename(file);
                    fileLine += " (";
                    fileLine += filename;
                    fileLine += ':';
                    fileLine += std::to_string(line);
                    fileLine += ')';
                }

                m_FileStream << fileLine << std::endl;
            }
            else
            {
                m_FileStream << t_MessageBuffer << std::endl;
            }
        }
    }
}

void Inkogger::setLogToFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_FileStream.is_open()) {
        m_FileStream.close();
    }

    m_LogToFile.store(!filepath.empty(), std::memory_order_relaxed);

    if (!filepath.empty()) {
        m_FileStream.open(filepath, std::ios::out | std::ios::app);
        if (!m_FileStream.is_open()) {
            // Fall back to console logging only
            m_LogToFile.store(false, std::memory_order_relaxed);
            // Log the error (without going into recursion)
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

LogManager::LogManager()
    : m_GlobalLevel(LogLevel::INFO),
    m_GlobalUseColors(true)
{
    // Empty
}

std::shared_ptr<IInkogger> LogManager::getLogger(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Loggers.find(name);
    if (it != m_Loggers.end()) {
        return it->second;
    }

    auto logger = std::make_shared<Inkogger>(name);
    logger->setLevel(m_GlobalLevel);

    if (!m_GlobalFilePath.empty()) {
        logger->setLogToFile(m_GlobalFilePath);
    }

    logger->setUseColors(m_GlobalUseColors);

    m_Loggers[name] = logger;
    return logger;
}

void LogManager::setGlobalLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_GlobalLevel = level;

    for (auto& pair : m_Loggers) {
        pair.second->setLevel(level);
    }
}

void LogManager::setLogToFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_GlobalFilePath = filepath;

    for (auto& pair : m_Loggers) {
        pair.second->setLogToFile(filepath);
    }
}

void LogManager::setUseColors(bool useColors)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_GlobalUseColors = useColors;

    for (auto& pair : m_Loggers) {
        pair.second->setUseColors(useColors);
    }
}

}  // namespace ink
