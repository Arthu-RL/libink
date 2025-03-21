#ifndef ENHANCEDJSONUTILS_H
#define ENHANCEDJSONUTILS_H

#pragma once

#include <string>
#include <vector>

#include "EnhancedJson.h"

namespace vac {

class EnhancedJsonUtils {
public:
    // Factory methods
    static EnhancedJson array();
    static EnhancedJson object();
    static EnhancedJson meta_info();

    // File operations
    static EnhancedJson loadFromFile(const std::string& filePath);
    static vac_bool saveToFile(const EnhancedJson& json, const std::string& filePath, vac_bool pretty = true, vac_i8 indent = 4);
    static EnhancedJson loadFromString(const std::string& jsonStr);

    // Serialization
    static std::string toString(const EnhancedJson& json, vac_bool pretty = false, vac_i8 indent = 4);
    static std::vector<uint8_t> toBinary(const EnhancedJson& json, const std::string& format = "cbor");
    static EnhancedJson fromBinary(const std::vector<uint8_t>& data, const std::string& format = "cbor");

    // Type checking utilities
    static vac_bool isArray(const EnhancedJson& json);
    static vac_bool isObject(const EnhancedJson& json);
    static vac_bool isNull(const EnhancedJson& json);
    static vac_bool isNumber(const EnhancedJson& json);
    static vac_bool isString(const EnhancedJson& json);
    static vac_bool isBoolean(const EnhancedJson& json);

    // Array utilities
    static vac_size size(const EnhancedJson& json);
    static vac_bool hasKey(const EnhancedJson& json, const std::string& key);
    static std::vector<std::string> getKeys(const EnhancedJson& json);

    // Merge and patch utilities
    static EnhancedJson merge(const EnhancedJson& a, const EnhancedJson& b);
    static EnhancedJson diff(const EnhancedJson& source, const EnhancedJson& target);
    static EnhancedJson patch(const EnhancedJson& source, const EnhancedJson& patch);

    // Schema validation
    static vac_bool validate(const EnhancedJson& json, const EnhancedJson& schema);

    // Debug utilities
    static std::string getTypeName(const EnhancedJson& json);
    // static vac_bool compareJson(const EnhancedJson& a, const EnhancedJson& b, vac_bool ignoreOrder = false); TODO
};


} // namespace aura3d

#endif // ENHANCEDJSONUTILS_H
