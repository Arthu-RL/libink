#include "../include/ink/Inkogger.h"

#include <sstream>
#include <chrono>
#include <iomanip>
#include <cstring>

namespace ink {

Inkogger::Inkogger(const std::string& name)
    : m_Name(name), m_Level(LogLevel::INFO), m_UseColors(true), m_LogToFile(false)
{
}

Inkogger::~Inkogger()
{
    if (m_FileStream.is_open())
    {
        m_FileStream.close();
    }
}

void Inkogger::setName(const std::string& name)
{
    m_Name = name;
}

void Inkogger::setLevel(LogLevel level)
{
    m_Level = level;
}

ink_bool Inkogger::isEnabled(LogLevel level) const
{
    return level <= m_Level;
}

std::string Inkogger::getColorForLevel(LogLevel level) const
{
    if (!m_UseColors) return "";

    return MAP_COLORS_FOR_LEVEL[static_cast<size_t>(level)].color;
}

std::string Inkogger::getLevelString(LogLevel level) const
{
    return MAP_COLORS_FOR_LEVEL[static_cast<size_t>(level)].desc;
}

std::string Inkogger::getCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Inkogger::log(LogLevel level, const std::string& message)
{
    if (level == LogLevel::OFF && message.size() > 0)
    {
        const char* result = message.data();
        // Output to console - use fwrite for better performance than cout
        fwrite(result, 1, message.size(), stdout);
        fwrite("\n", 1, 1, stdout);
        fflush(stdout);

        // Output to file if enabled
        if (m_LogToFile && m_FileStream.is_open())
        {
            m_FileStream << result << std::endl;
        }
    }
}

void Inkogger::log(LogLevel level, const std::string& message, const char* file, ink_u32 line)
{
    // This check is redundant with the macro check, but kept for safety
    if (!isEnabled(level)) return;

    // Get just the filename from the path - avoid string allocation in hot path
    const char* filename = file;
    const char* lastSlash = strrchr(file, '/');
    const char* lastBackslash = strrchr(file, '\\');

    if (lastSlash != nullptr) {
        filename = lastSlash + 1;
    } else if (lastBackslash != nullptr) {
        filename = lastBackslash + 1;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);

    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = getLevelString(level);
    std::string color = getColorForLevel(level);
    std::string reset = m_UseColors ? ink::LoggerColors::RESET : "";
    std::string lineStr = std::to_string(line);
    size_t filenameLen = std::strlen(filename);

    // Estimate size for reserve
    size_t totalSize =
        2 + timestamp.size() + 1 + color.size() +
        2 + levelStr.size() + reset.size() +
        3 + m_Name.size() +
        2 + message.size() +
        2 + filenameLen + 1 + lineStr.size() + 1;

    std::string result;
    result.reserve(totalSize);  // Prevent reallocations

    // Format: [timestamp] [level] [name]: message (file:line)
    result += '[' + timestamp + "] " +
              color + '[' + levelStr + ']' + reset +
              " [" + m_Name + "]: " +
              message + " (" + filename + ':' + lineStr + ')';

    size_t len = result.size();

    if (len > 0)
    {
        // Output to console - use fwrite for better performance than cout
        fwrite(result.data(), 1, len, stdout);
        fwrite("\n", 1, 1, stdout);
        fflush(stdout);

        // Output to file if enabled
        if (m_LogToFile && m_FileStream.is_open())
        {
            m_FileStream << '[' + timestamp + "] " +
                            '[' + levelStr + ']' +
                            " [" + m_Name + "]: " +
                            message + " (" + filename + ':' + lineStr + ')' << std::endl;
        }
    }
}

void Inkogger::setLogToFile(const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_FileStream.is_open()) {
        m_FileStream.close();
    }

    m_LogToFile = !filepath.empty();

    if (m_LogToFile) {
        m_FileStream.open(filepath, std::ios::out | std::ios::app);
        if (!m_FileStream.is_open()) {
            INK_ERROR << "Failed to open log file: " << filepath;
            m_LogToFile = false;
        }
    }
}

void Inkogger::setUseColors(ink_bool useColors)
{
    m_UseColors = useColors;
}

// LogManager implementation
std::shared_ptr<Inkogger> LogManager::getLogger(const std::string& name)
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

void LogManager::setUseColors(ink_bool useColors)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_GlobalUseColors = useColors;

    for (auto& pair : m_Loggers) {
        pair.second->setUseColors(useColors);
    }
}

}
