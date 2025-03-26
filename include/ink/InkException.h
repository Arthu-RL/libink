#ifndef INKEXCEPTION_H
#define INKEXCEPTION_H

/**
 * @file InkException.hpp
 * @brief Exception handling for INK library
 *
 * This file contains a hierarchy of exception classes for the INK library
 * that provide advanced error handling capabilities.
 */

#pragma once

#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "ink/ink_base.hpp"

namespace ink {

/**
 * @brief Base exception class for INK library
 *
 * This class serves as the foundation for all INK exceptions,
 * providing common functionality and a clean interface.
 */
class INK_API INKException : public std::exception {
public:
    // =============== Constructors ===============

    /**
     * @brief Create a new exception
     * @param message The error message
     * @param location Source location information (auto-captured)
     */
    explicit INKException(
        const std::string& message,
        const char* file = __FILE__,
        ink_u32 line = __LINE__,
        const char* function = __FUNCTION__
        ) :
        m_message(message),
        m_file(file),
        m_function(function),
        m_line(line),
        m_timestamp(std::chrono::system_clock::now())
    {
        buildFullMessage();
        captureStackTrace();
    }

    /**
     * @brief Create a new exception with a result code
     * @param result The result code
     * @param message The error message
     * @param file Source file name
     * @param line Source line number
     * @param function Source function name
     */
    explicit INKException(
        ink_i32 result_code,
        const std::string& message,
        const char* file = __FILE__,
        ink_u32 line = __LINE__,
        const char* function = __FUNCTION__
        ) :
        m_result_code(result_code),
        m_message(message),
        m_file(file),
        m_function(function),
        m_line(line),
        m_timestamp(std::chrono::system_clock::now())
    {
        buildFullMessage();
        captureStackTrace();
    }

    // =============== std::exception Interface ===============

    /**
     * @brief Get the full error message with all details
     * @return C-string containing the formatted error message
     */
    virtual const char* what() const noexcept override {
        return m_fullMessage.c_str();
    }

    /**
     * @brief Virtual destructor to allow proper cleanup of derived classes
     */
    virtual ~INKException() noexcept = default;

    // =============== Virtual Type Methods ===============

    /**
     * @brief Get the exception type name
     * @return String representing the exception type
     */
    virtual std::string typeName() const {
        return "Generic";
    }

    // =============== Accessors ===============

    /**
     * @brief Get the basic error message
     * @return The error message
     */
    const std::string& message() const noexcept {
        return m_message;
    }

    /**
     * @brief Get the result code
     * @return The result code
     */
    ink_i32 result_code() const noexcept {
        return m_result_code;
    }

    /**
     * @brief Get the file where the exception was thrown
     * @return The file name
     */
    const std::string& file() const noexcept {
        return m_file;
    }

    /**
     * @brief Get the function where the exception was thrown
     * @return The function name
     */
    const std::string& function() const noexcept {
        return m_function;
    }

    /**
     * @brief Get the line where the exception was thrown
     * @return The line number
     */
    uint32_t line() const noexcept {
        return m_line;
    }

    // Column information is not available in C++17 and below

    /**
     * @brief Get the timestamp when the exception was created
     * @return The timestamp
     */
    std::chrono::system_clock::time_point timestamp() const noexcept {
        return m_timestamp;
    }

    /**
     * @brief Get the stack trace
     * @return Vector of strings representing the stack trace
     */
    const std::vector<std::string>& stackTrace() const noexcept {
        return m_stackTrace;
    }

    // =============== Context Handling ===============

    /**
     * @brief Add context information to the exception
     * @param key The context key
     * @param value The context value
     * @return Reference to this exception (for chaining)
     */
    template<typename T>
    INKException& addContext(const std::string& key, const T& value) {
        std::ostringstream oss;
        oss << value;
        m_context[key] = oss.str();
        buildFullMessage(); // Rebuild the full message
        return *this;
    }

    /**
     * @brief Get context value by key
     * @param key The context key
     * @return The context value or empty string if not found
     */
    std::string getContext(const std::string& key) const {
        auto it = m_context.find(key);
        if (it != m_context.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Check if a context key exists
     * @param key The context key
     * @return True if the key exists
     */
    ink_bool hasContext(const std::string& key) const {
        return m_context.find(key) != m_context.end();
    }

    // =============== Helper Methods ===============

    /**
     * @brief Convert the exception to a formatted string
     * @param includeStackTrace Whether to include the stack trace
     * @return Formatted string representation of the exception
     */
    virtual std::string toString(ink_bool includeStackTrace = true) const {
        return m_fullMessage + (includeStackTrace ? formatStackTrace() : "");
    }

    /**
     * @brief Create a nested exception (for wrapping another exception)
     * @param message The outer exception message
     * @param innerException The inner exception to wrap
     * @param location Source location information
     * @return A new INKException with nested information
     */
    static INKException nested(
        const std::string& message,
        const std::exception& innerException,
        const char* file = __FILE__,
        ink_u32 line = __LINE__,
        const char* function = __FUNCTION__
        ) {
        INKException ex(message, file, line, function);
        ex.addContext("InnerException", innerException.what());
        return ex;
    }

protected:
    /**
     * @brief Build the full error message with all details
     * Override this in derived classes to customize the message format
     */
    virtual void buildFullMessage() {
        std::ostringstream oss;
        oss << typeName() << " Exception: " << m_message << "\n"
            << "  Location: " << m_file << ":" << m_line;

        if (!m_function.empty()) {
            oss << " in " << m_function;
        }
        oss << "\n";

        // Add result code if available
        if (m_result_code != static_cast<ink_i32>(ink_result_t::SUCCESS)) {
            oss << "  Result Code: " << static_cast<int>(m_result_code) << "\n";
        }

        // Add timestamp
        auto timeT = std::chrono::system_clock::to_time_t(m_timestamp);
        char timeBuf[32];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&timeT));
        oss << "  Time: " << timeBuf << "\n";

        // Add context if available
        if (!m_context.empty()) {
            oss << "  Context:\n";
            for (const auto& [key, value] : m_context) {
                oss << "    " << key << ": " << value << "\n";
            }
        }

        m_fullMessage = oss.str();
    }

    /**
     * @brief Format the stack trace for display
     * @return Formatted stack trace string
     */
    std::string formatStackTrace() const {
        if (m_stackTrace.empty()) {
            return {};
        }

        std::ostringstream oss;
        oss << "\nStack Trace:\n";
        for (size_t i = 0; i < m_stackTrace.size(); ++i) {
            oss << "  #" << i << " " << m_stackTrace[i] << "\n";
        }
        return oss.str();
    }

    /**
     * @brief Capture the stack trace (platform-specific)
     */
    void captureStackTrace() {
        // This is a simplified implementation. In production code,
        // you would use platform-specific techniques to capture a real stack trace.

        // For now, we'll just add the current function
        m_stackTrace.push_back(m_function);

        // In a real implementation, you would use:
        // - Windows: CaptureStackBackTrace() or StackWalk64()
        // - Linux: backtrace() and backtrace_symbols()
        // - Or a library like Boost.Stacktrace
    }

protected:
    // Protected data members accessible to derived classes
    ink_i32 m_result_code = static_cast<ink_i32>(ink_result_t::SUCCESS);
    std::string m_message;
    std::string m_file;
    std::string m_function;
    ink_u32 m_line;
    // No column number in C++17 and below
    std::chrono::system_clock::time_point m_timestamp;
    std::vector<std::string> m_stackTrace;
    std::unordered_map<std::string, std::string> m_context;
    std::string m_fullMessage;
};

// =============== Helper Macros ===============

/**
 * @brief Throw a generic exception with current source location
 * @param message The error message
 */
#define INK_THROW(message) \
throw ink::INKException((message), __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Throw a specific type of exception with current source location
 * @param ExClass The exception class
 * @param ... Arguments to pass to the exception constructor
 */
#define INK_THROW_EX(ExClass, ...) \
    throw ink::ExClass(__VA_ARGS__)

/**
 * @brief Throw if condition is not met
 * @param condition The condition to check
 * @param message The error message
 */
#define INK_THROW_IF(condition, message) \
    if (INK_UNLIKELY(!(condition))) { INK_THROW(message); }

/**
 * @brief Throw a specific exception type if condition is not met
 * @param condition The condition to check
 * @param ExClass The exception class
 * @param ... Arguments to pass to the exception constructor
 */
#define INK_THROW_IF_EX(condition, ExClass, ...) \
if (INK_UNLIKELY(!(condition))) { INK_THROW_EX(ExClass, __VA_ARGS__); }

/**
 * @brief Create a nested exception wrapping the current exception
 * @param message The outer exception message
 */
#define INK_NEST_EXCEPTION(message) \
catch (const std::exception& e) { \
        throw ink::INKException::nested((message), e, __FILE__, __LINE__, __FUNCTION__); \
}

/**
 * @brief Execute code and catch any exceptions, wrapping them with context
 * @param code The code to execute
 * @param message The error message for wrapper exception
 */
#define INK_TRY_WRAP(code, message) \
try { \
        code; \
} INK_NEST_EXCEPTION(message)

} // namespace ink

#endif // INKEXCEPTION_H
