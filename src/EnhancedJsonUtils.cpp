#include "../include/ink/EnhancedJsonUtils.h"
#include <iostream>

namespace ink {

EnhancedJson EnhancedJsonUtils::meta_info() {
    return EnhancedJson(nlohmann::json::meta());
}

// File operations
EnhancedJson EnhancedJsonUtils::loadFromFile(const std::string& filePath) {
    return EnhancedJson::loadFromFile(filePath);
}

bool EnhancedJsonUtils::saveToFile(const EnhancedJson& json, const std::string& filePath, bool pretty, i8 indent) {
    return json.saveToFile(filePath, pretty, indent);
}

EnhancedJson EnhancedJsonUtils::loadFromString(const std::string& jsonStr) {
    try {
        return EnhancedJson(nlohmann::json::parse(jsonStr));
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[ERROR] JSON parse error in string: " << e.what() << std::endl;
        return EnhancedJson();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Error parsing JSON string: " << e.what() << std::endl;
        return EnhancedJson();
    }
}

// Serialization
std::string EnhancedJsonUtils::toString(const EnhancedJson& json, bool pretty, i8 indent) {
    return pretty ? json.toPrettyString(indent) : json.toCompactString();
}

std::vector<u8> EnhancedJsonUtils::toBinary(const EnhancedJson& json, const std::string& format) {
    try {
        if (format == "cbor") {
            return EnhancedJson::to_cbor(json);
        } else if (format == "msgpack") {
            return EnhancedJson::to_msgpack(json);
        } else if (format == "bson") {
            return EnhancedJson::to_bson(json);
        } else {
            std::cerr << "[ERROR] Unsupported binary format: " << format << std::endl;
            return {};
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Error converting JSON to binary format: " << e.what() << std::endl;
        return {};
    }
}

EnhancedJson EnhancedJsonUtils::fromBinary(const std::vector<u8>& data, const std::string& format) {
    try {
        if (format == "cbor") {
            return EnhancedJson::from_cbor(data);
        } else if (format == "msgpack") {
            return EnhancedJson::from_msgpack(data);
        } else if (format == "bson") {
            return EnhancedJson::from_bson(data);
        } else {
            std::cerr << "[ERROR] Unsupported binary format: " << format << std::endl;
            return EnhancedJson();
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Error converting binary to JSON: " << e.what() << std::endl;
        return EnhancedJson();
    }
}

// Type checking utilities
bool EnhancedJsonUtils::isArray(const EnhancedJson& json) {
    return json.is_array();
}

bool EnhancedJsonUtils::isObject(const EnhancedJson& json) {
    return json.is_object();
}

bool EnhancedJsonUtils::isNull(const EnhancedJson& json) {
    return json.is_null();
}

bool EnhancedJsonUtils::isNumber(const EnhancedJson& json) {
    return json.is_number();
}

bool EnhancedJsonUtils::isString(const EnhancedJson& json) {
    return json.is_string();
}

bool EnhancedJsonUtils::isBoolean(const EnhancedJson& json) {
    return json.is_boolean();
}

// Array utilities
size_t EnhancedJsonUtils::size(const EnhancedJson& json) {
    return json.size();
}

bool EnhancedJsonUtils::hasKey(const EnhancedJson& json, const std::string& key) {
    return json.has(key);
}

std::vector<std::string> EnhancedJsonUtils::getKeys(const EnhancedJson& json) {
    return json.keys();
}

// Merge and patch utilities
EnhancedJson EnhancedJsonUtils::merge(const EnhancedJson& a, const EnhancedJson& b) {
    EnhancedJson result = a;
    result.merge(b);
    return result;
}

EnhancedJson EnhancedJsonUtils::diff(const EnhancedJson& source, const EnhancedJson& target) {
    return EnhancedJson(nlohmann::json::diff(source, target));
}

EnhancedJson EnhancedJsonUtils::patch(const EnhancedJson& source, const EnhancedJson& patchData) {
    return EnhancedJson(source.patch(patchData));
}

std::string EnhancedJsonUtils::getTypeName(const EnhancedJson& json) {
    return json.type_name();
}

} // namespace aura3d
