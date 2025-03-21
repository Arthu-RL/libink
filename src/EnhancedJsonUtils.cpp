#include "../include/vac/EnhancedJsonUtils.h"
#include <iostream>

namespace vac {

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

vac_bool EnhancedJsonUtils::saveToFile(const EnhancedJson& json, const std::string& filePath, vac_bool pretty, vac_i8 indent) {
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
std::string EnhancedJsonUtils::toString(const EnhancedJson& json, vac_bool pretty, vac_i8 indent) {
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
vac_bool EnhancedJsonUtils::isArray(const EnhancedJson& json) {
    return json.is_array();
}

vac_bool EnhancedJsonUtils::isObject(const EnhancedJson& json) {
    return json.is_object();
}

vac_bool EnhancedJsonUtils::isNull(const EnhancedJson& json) {
    return json.is_null();
}

vac_bool EnhancedJsonUtils::isNumber(const EnhancedJson& json) {
    return json.is_number();
}

vac_bool EnhancedJsonUtils::isString(const EnhancedJson& json) {
    return json.is_string();
}

vac_bool EnhancedJsonUtils::isBoolean(const EnhancedJson& json) {
    return json.is_boolean();
}

// Array utilities
vac_size EnhancedJsonUtils::size(const EnhancedJson& json) {
    return json.size();
}

vac_bool EnhancedJsonUtils::hasKey(const EnhancedJson& json, const std::string& key) {
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
vac_bool EnhancedJsonUtils::validate(const EnhancedJson& json, const EnhancedJson& schema) {
    return json.isValid(schema);
}

std::string EnhancedJsonUtils::getTypeName(const EnhancedJson& json) {
    return json.type_name();
}

} // namespace aura3d
