#ifndef ENHANCED_JSON_H
#define ENHANCED_JSON_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <variant>

#include "ink/ink_base.hpp"
#include "InkType.h"

namespace ink {

// Forward declaration for Query class
class INK_API JsonQuery;

/**
 * @brief Enhanced JSON class that extends nlohmann::json with additional functionality
 *
 * This class inherits from nlohmann::json and adds extra features:
 * - Safe accessors that don't throw exceptions
 * - Path-based access using dot notation
 * - Automatic type conversions
 * - Chained queries and filters
 * - Transformation and mapping functions
 * - Extended validation capabilities
 */
class INK_API EnhancedJson : public nlohmann::json {
public:
    // Constructors - inherit all constructors from the base class
    using nlohmann::json::json;

    // Additional constructors
    EnhancedJson() : nlohmann::json() {}

    // Copy constructor from nlohmann::json
    EnhancedJson(const nlohmann::json& other) : nlohmann::json(other) {}

    // Doesn't work, nlohmann::json only accepts strings, and converts the type when you use T get<T> function
    EnhancedJson(const InkType& inkType) {
        auto variant = inkType.toVariant();

        std::visit([this](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (!std::is_same_v<T, void*> && !std::is_same_v<T, std::monostate>) {
                *this = nlohmann::json(value);
            }
            else {
                *this = nlohmann::json();
            }
        }, variant);
    }

    // Move constructor from nlohmann::json
    EnhancedJson(nlohmann::json&& other) noexcept : nlohmann::json(std::move(other)) {}

    // Operator overloads
    EnhancedJson& operator=(const nlohmann::json& other) {
        nlohmann::json::operator=(other);
        return *this;
    }

    EnhancedJson& operator=(nlohmann::json&& other) noexcept {
        nlohmann::json::operator=(std::move(other));
        return *this;
    }

    //
    // Safe accessors
    //

    /**
     * @brief Get value at specified key without throwing exception
     * @param key The key to look up
     * @param defaultValue Value to return if key is not found
     * @return Value at key or default value
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        try {
            if (is_object() && contains(key)) {
                return at(key).get<T>();
            }
        } catch (...) {
            // Swallow the exception and return default value
        }

        return defaultValue;
    }

    // Specialization for EnhancedJson
    EnhancedJson get(const std::string& key, const EnhancedJson& defaultValue = EnhancedJson()) const {
        try {
            if (is_object() && contains(key)) {
                return EnhancedJson(at(key));
            }
        } catch (...) {
            // Swallow the exception and return default value
        }

        return defaultValue;
    }

    /**
     * @brief Get value at array index without throwing exception
     * @param index The array index
     * @param defaultValue Value to return if index is out of bounds
     * @return Value at index or default value
     */
    template<typename T>
    T get(size_t index, const T& defaultValue = T()) const {
        try {
            if (is_array() && index < size()) {
                return at(index).get<T>();
            }
        } catch (...) {
            // Swallow the exception and return default value
        }

        return defaultValue;
    }

    // Specialization for EnhancedJson
    EnhancedJson get(size_t index, const EnhancedJson& defaultValue = EnhancedJson()) const {
        try {
            if (is_array() && index < size()) {
                return EnhancedJson(at(index));
            }
        } catch (...) {
            // Swallow the exception and return default value
        }

        return defaultValue;
    }

    /**
     * @brief Get value using dot notation path
     * @param path Path to the value (e.g., "user/address/street")
     * @param defaultValue Value to return if path is not found
     * @return Value at path or default value
     */
    template<typename T>
    T getPath(const std::string& path, const T& defaultValue = T()) const {
        try
        {
            return value(nlohmann::json::json_pointer(path), defaultValue);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    // Specialization for EnhancedJson
    EnhancedJson getPath(const std::string& path, const EnhancedJson& defaultValue = EnhancedJson()) const {
        try
        {
            return EnhancedJson(at(nlohmann::json::json_pointer(path)));
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    /**
     * @brief Check if a key exists in the JSON object
     * @param key The key to check
     * @return True if key exists, false otherwise
     */
    bool has(const std::string& key) const {
        return is_object() && contains(key);
    }

    /**
     * @brief Check if a path exists in the JSON object
     * @param path The path to check (e.g., "user/address/street")
     * @return True if path exists, false otherwise
     */
    bool hasPath(const std::string& path) const {
        return is_object() && contains(nlohmann::json::json_pointer(path));
    }

    //
    // Array operations
    //

    /**
     * @brief Filter array elements based on a predicate
     * @param predicate Function that takes EnhancedJson and returns bool
     * @return New EnhancedJson array with filtered elements
     */
    EnhancedJson filter(ink::move_only_function<bool(const EnhancedJson&)> predicate) const {
        EnhancedJson result = array();

        if (!is_array()) {
            return result;
        }

        for (const auto& item : *this) {
            EnhancedJson enhancedItem = EnhancedJson(item);
            if (predicate(enhancedItem)) {
                result.push_back(item);
            }
        }

        return result;
    }

    /**
     * @brief Map array elements using a transform function
     * @param transform Function that takes EnhancedJson and returns a transformed value
     * @return New EnhancedJson array with transformed elements
     */
    template<typename T>
    EnhancedJson map(ink::move_only_function<T(const EnhancedJson&)> transform) const {
        EnhancedJson result = array();

        if (!is_array()) {
            return result;
        }

        for (const auto& item : *this) {
            EnhancedJson enhancedItem = EnhancedJson(item);
            result.push_back(transform(enhancedItem));
        }

        return result;
    }

    /**
     * @brief Reduce array elements to a single value
     * @param initial Initial value
     * @param reducer Function that accumulates values
     * @return Reduced value
     */
    template<typename T>
    T reduce(T initial, ink::move_only_function<T(T, const EnhancedJson&)> reducer) const {
        T result = initial;

        if (!is_array()) {
            return result;
        }

        for (const auto& item : *this) {
            EnhancedJson enhancedItem = EnhancedJson(item);
            result = reducer(result, enhancedItem);
        }

        return result;
    }

    /**
     * @brief Find first element that matches a predicate
     * @param predicate Function that tests elements
     * @return First matching element or null
     */
    EnhancedJson find(ink::move_only_function<bool(const EnhancedJson&)> predicate) const {
        if (!is_array()) {
            return EnhancedJson(nullptr);
        }

        for (const auto& item : *this) {
            EnhancedJson enhancedItem = EnhancedJson(item);
            if (predicate(enhancedItem)) {
                return enhancedItem;
            }
        }

        return EnhancedJson(nullptr);
    }

    /**
     * @brief Find all elements that match a predicate
     * @param predicate Function that tests elements
     * @return Array of matching elements
     */
    EnhancedJson findAll(ink::move_only_function<bool(const EnhancedJson&)> predicate) const {
        return filter(std::move(predicate));
    }

    /**
     * @brief Check if any array element matches a predicate
     * @param predicate Function that tests elements
     * @return True if any element matches, false otherwise
     */
    bool any(ink::move_only_function<bool(const EnhancedJson&)> predicate) const {
        if (!is_array()) {
            return false;
        }

        for (const auto& item : *this) {
            EnhancedJson enhancedItem = EnhancedJson(item);
            if (predicate(enhancedItem)) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Check if all array elements match a predicate
     * @param predicate Function that tests elements
     * @return True if all elements match, false otherwise
     */
    bool all(ink::move_only_function<bool(const EnhancedJson&)> predicate) const {
        if (!is_array() || empty()) {
            return false;
        }

        for (const auto& item : *this) {
            EnhancedJson enhancedItem = EnhancedJson(item);
            if (!predicate(enhancedItem)) {
                return false;
            }
        }

        return true;
    }

    //
    // Object operations
    //

    /**
     * @brief Set a value at the specified path, creating intermediate objects if needed
     * @param path Path to set the value at (e.g., "user.address.street")
     * @param value Value to set
     * @return Reference to this object
     */
    template<typename T>
    EnhancedJson& setPath(const std::string& path, const T& value) {
        (*this)[nlohmann::json::json_pointer(path)] = value;
        return *this;
    }

    /**
     * @brief Merge this JSON object with another
     * @param other JSON object to merge with
     * @param overwrite Whether to overwrite existing values
     * @return Reference to this object
     */
    void merge(EnhancedJson other, bool overwrite = true) {
        if (!is_object() || !other.is_object())
        {
            return;
        }

        std::vector<std::pair<nlohmann::json*, nlohmann::json*>> stack;
        stack.reserve(32);
        stack.push_back({this, &other});

        while (!stack.empty())
        {
            auto [target, source] = stack.back();
            stack.pop_back();

            for (auto&& [key, value] : source->items())
            {
                auto targetIt = target->find(key);

                if (targetIt != target->end())
                {
                    auto& targetValue = *targetIt;

                    if (targetValue.is_object() && value.is_object())
                    {
                        stack.push_back({&targetValue, &value});
                    }
                    else if (overwrite)
                    {
                        targetValue = std::move(value);
                    }
                }
                else
                {
                    (*target)[key] = std::move(value);
                }
            }
        }
    }

    /**
     * @brief Get all keys of the JSON object
     * @return Vector of keys
     */
    std::vector<std::string> keys() const {
        std::vector<std::string> result;

        if (!is_object()) {
            return result;
        }

        for (auto it = begin(); it != end(); ++it) {
            result.push_back(it.key());
        }

        return result;
    }

    /**
     * @brief Remove a key from the JSON object
     * @param key Key to remove
     * @return True if key was removed, false otherwise
     */
    bool removeKey(const std::string& key) {
        if (!is_object() || !contains(key)) {
            return false;
        }

        erase(key);
        return true;
    }

    /**
     * @brief Remove a value at the specified path
     * @param path Path to the value to remove
     * @return True if value was removed, false otherwise
     */
    bool removePath(const std::string& path) {
        size_t pos = path.find('.');

        // No more dots, this is the final segment
        if (pos == std::string::npos) {
            return removeKey(path);
        }

        // Get the first segment and remaining path
        std::string firstSegment = path.substr(0, pos);
        std::string remainingPath = path.substr(pos + 1);

        // If the first segment exists and is an object, recursively remove from it
        if (is_object() && contains(firstSegment) && at(firstSegment).is_object()) {
            EnhancedJson nextJson = EnhancedJson((*this)[firstSegment]);
            bool result = nextJson.removePath(remainingPath);
            (*this)[firstSegment] = nextJson;
            return result;
        }

        return false;
    }

    //
    // Query DSL - fluent interface for JSON queries
    //

    /**
     * @brief Start a query at the specified path
     * @param path Path to start query from
     * @return Query object for fluent interface
     */
    JsonQuery query(const std::string& path = "") const;

    //
    // Utility methods
    //

    /**
     * @brief Get a pretty-printed string representation
     * @param indent Number of spaces for indentation
     * @return Formatted JSON string
     */
    std::string toPrettyString(int indent = 4) const {
        return dump(indent);
    }

    /**
     * @brief Get a compact string representation
     * @return Compact JSON string
     */
    std::string toCompactString() const {
        return dump();
    }

    /**
     * @brief Load EnhancedJson from a file
     * @param filepath Path to the JSON file
     * @return EnhancedJson object
     */
    static EnhancedJson loadFromFile(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                return EnhancedJson();
            }

            EnhancedJson json;
            file >> json;
            return json;
        } catch (...) {
            return EnhancedJson();
        }
    }

    /**
     * @brief Save EnhancedJson to a file
     * @param filepath Path to save the JSON file
     * @param pretty Whether to pretty-print the JSON
     * @param indent Number of spaces for indentation
     * @return True if successful, false otherwise
     */
    bool saveToFile(const std::string& filepath, bool pretty = true, int indent = 4) const {
        try {
            std::ofstream file(filepath);
            if (!file.is_open()) {
                return false;
            }

            if (pretty) {
                file << dump(indent);
            } else {
                file << dump();
            }

            return true;
        } catch (...) {
            return false;
        }
    }
};

/**
 * @brief Query class for fluent interface to JSON data
 */
class INK_API JsonQuery {
public:
    JsonQuery(const EnhancedJson* root, const std::string& path = "")
        : m_root(root) {
        if (!path.empty()) {
            m_target = root->getPath(path);
        } else {
            m_target = *root;
        }
    }

    /**
     * @brief Get a value from the current target
     * @param key Key to get value for
     * @param defaultValue Default value if key doesn't exist
     * @return Value at key or default value
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        return m_target.get<T>(key, defaultValue);
    }

    /**
     * @brief Get a value from the current target at index
     * @param index Array index
     * @param defaultValue Default value if index is invalid
     * @return Value at index or default value
     */
    template<typename T>
    T get(size_t index, const T& defaultValue = T()) const {
        return m_target.get<T>(index, defaultValue);
    }

    /**
     * @brief Check if current target has a key
     * @param key Key to check
     * @return True if key exists, false otherwise
     */
    bool has(const std::string& key) const {
        return m_target.has(key);
    }

    /**
     * @brief Select a new path relative to current target
     * @param path Path to select
     * @return New query with updated target
     */
    JsonQuery select(const std::string& path) const {
        if (path.empty()) {
            return *this;
        }

        if (m_target.empty()) {
            return JsonQuery(m_root, path);
        }

        EnhancedJson newTarget = m_target.getPath(path);
        return JsonQuery(m_root, newTarget);
    }

    /**
     * @brief Filter the current target if it's an array
     * @param predicate Function to test elements
     * @return New query with filtered target
     */
    JsonQuery filter(ink::move_only_function<bool(const EnhancedJson&)> predicate) const {
        EnhancedJson filtered = m_target.filter(std::move(predicate));
        return JsonQuery(m_root, filtered);
    }

    /**
     * @brief Get the first element of the current target if it's an array
     * @return New query with first element as target
     */
    JsonQuery first() const {
        if (m_target.is_array() && !m_target.empty()) {
            EnhancedJson firstElement = m_target[0];
            return JsonQuery(m_root, firstElement);
        }
        return JsonQuery(m_root, EnhancedJson());
    }

    /**
     * @brief Get the last element of the current target if it's an array
     * @return New query with last element as target
     */
    JsonQuery last() const {
        if (m_target.is_array() && !m_target.empty()) {
            EnhancedJson lastElement = m_target[m_target.size() - 1];
            return JsonQuery(m_root, lastElement);
        }
        return JsonQuery(m_root, EnhancedJson());
    }

    /**
     * @brief Get current query target
     * @return Current target
     */
    const EnhancedJson& value() const {
        return m_target;
    }

private:
    const EnhancedJson* m_root;
    EnhancedJson m_target;

    // Constructor for internal use
    JsonQuery(const EnhancedJson* root, const EnhancedJson& target)
        : m_root(root), m_target(target) {}
};

// Implementation of query method
INK_INLINE JsonQuery EnhancedJson::query(const std::string& path) const {
    return JsonQuery(this, path);
}

} // namespace aura3d

#endif // ENHANCED_JSON_H
