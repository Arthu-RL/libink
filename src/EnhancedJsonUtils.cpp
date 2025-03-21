#include "../include/ink/EnhancedJsonUtils.h"
#include <iostream>

namespace ink {

// Factory methods
EnhancedJson EnhancedJsonUtils::array() {
    return EnhancedJson::createArray();
}

EnhancedJson EnhancedJsonUtils::object() {
    return EnhancedJson::createObject();
}

EnhancedJson EnhancedJsonUtils::meta_info() {
    return EnhancedJson(nlohmann::json::meta());
}

// File operations
EnhancedJson EnhancedJsonUtils::loadFromFile(const std::string& filePath) {
    return EnhancedJson::loadFromFile(filePath);
}

ink_bool EnhancedJsonUtils::saveToFile(const EnhancedJson& json, const std::string& filePath, ink_bool pretty, ink_i8 indent) {
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
std::string EnhancedJsonUtils::toString(const EnhancedJson& json, ink_bool pretty, ink_i8 indent) {
    return pretty ? json.toPrettyString(indent) : json.toCompactString();
}

std::vector<uint8_t> EnhancedJsonUtils::toBinary(const EnhancedJson& json, const std::string& format) {
    try {
        if (format == "cbor") {
            return json.toCBOR();
        } else if (format == "msgpack") {
            return json.toMsgPack();
        } else if (format == "bson") {
            return json.toBSON();
        } else {
            std::cerr << "[ERROR] Unsupported binary format: " << format << std::endl;
            return {};
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Error converting JSON to binary format: " << e.what() << std::endl;
        return {};
    }
}

EnhancedJson EnhancedJsonUtils::fromBinary(const std::vector<uint8_t>& data, const std::string& format) {
    try {
        if (format == "cbor") {
            return EnhancedJson::fromCBOR(data);
        } else if (format == "msgpack") {
            return EnhancedJson::fromMsgPack(data);
        } else if (format == "bson") {
            return EnhancedJson::fromBSON(data);
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
ink_bool EnhancedJsonUtils::isArray(const EnhancedJson& json) {
    return json.is_array();
}

ink_bool EnhancedJsonUtils::isObject(const EnhancedJson& json) {
    return json.is_object();
}

ink_bool EnhancedJsonUtils::isNull(const EnhancedJson& json) {
    return json.is_null();
}

ink_bool EnhancedJsonUtils::isNumber(const EnhancedJson& json) {
    return json.is_number();
}

ink_bool EnhancedJsonUtils::isString(const EnhancedJson& json) {
    return json.is_string();
}

ink_bool EnhancedJsonUtils::isBoolean(const EnhancedJson& json) {
    return json.is_boolean();
}

// Array utilities
ink_size EnhancedJsonUtils::size(const EnhancedJson& json) {
    return json.size();
}

ink_bool EnhancedJsonUtils::hasKey(const EnhancedJson& json, const std::string& key) {
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

// Schema validation
ink_bool EnhancedJsonUtils::validate(const EnhancedJson& json, const EnhancedJson& schema) {
    return json.isValid(schema);
}

std::string EnhancedJsonUtils::getTypeName(const EnhancedJson& json) {
    return json.type_name();
}

} // namespace aura3d
