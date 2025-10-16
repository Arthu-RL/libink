#ifndef INKTYPE_H
#define INKTYPE_H

#pragma once

#include "ink/ink_base.hpp"
#include <string>
#include <variant>
#include <cstring>

namespace ink {

class InkType {
public:
    enum class InkTypeId {
        Invalid,
        I8,
        I16,
        I32,
        I64,
        U8,
        U16,
        U32,
        U64,
        F32,
        F64,
        String,
        Bool,
        Char,
        Handle
    };

    using Variant = std::variant<
        std::monostate,
        i8, i16, i32, i64,
        u8, u16, u32, u64,
        f32, f64,
        std::string,
        bool,
        char,
        ink_h
    >;

    InkType() : m_type(InkTypeId::Invalid) { m_data.u64_value = 0; }

    InkType(i8 value) : m_type(InkTypeId::I8) { m_data.i64_value = value; }
    InkType(i16 value) : m_type(InkTypeId::I16) { m_data.i64_value = value; }
    InkType(i32 value) : m_type(InkTypeId::I32) { m_data.i64_value = value; }
    InkType(i64 value) : m_type(InkTypeId::I64) { m_data.i64_value = value; }
    InkType(u8 value) : m_type(InkTypeId::U8) { m_data.u64_value = value; }
    InkType(u16 value) : m_type(InkTypeId::U16) { m_data.u64_value = value; }
    InkType(u32 value) : m_type(InkTypeId::U32) { m_data.u64_value = value; }
    InkType(u64 value) : m_type(InkTypeId::U64) { m_data.u64_value = value; }
    InkType(f32 value) : m_type(InkTypeId::F32) { m_data.f32_value = value; }
    InkType(f64 value) : m_type(InkTypeId::F64) { m_data.f64_value = value; }
    InkType(bool value) : m_type(InkTypeId::Bool) { m_data.bool_value = value; }
    InkType(char value) : m_type(InkTypeId::Char) { m_data.char_value = value; }
    InkType(ink_h value) : m_type(InkTypeId::Handle) { m_data.handle_value = value; }

    InkType(const char* value) : m_type(InkTypeId::String) {
        m_data.string_value = new std::string(value);
    }

    InkType(const std::string& value) : m_type(InkTypeId::String) {
        m_data.string_value = new std::string(value);
    }

    InkType(std::string&& value) : m_type(InkTypeId::String) {
        m_data.string_value = new std::string(std::move(value));
    }

    InkType(const InkType& other) : m_type(other.m_type) {
        if (m_type == InkTypeId::String && other.m_data.string_value) {
            m_data.string_value = new std::string(*other.m_data.string_value);
        } else {
            m_data = other.m_data;
        }
    }

    InkType(InkType&& other) noexcept : m_data(other.m_data), m_type(other.m_type) {
        other.m_type = InkTypeId::Invalid;
        other.m_data.u64_value = 0;
    }

    ~InkType() {
        cleanup();
    }

    InkType& operator=(const InkType& other) {
        if (this != &other) {
            cleanup();
            m_type = other.m_type;
            if (m_type == InkTypeId::String && other.m_data.string_value) {
                m_data.string_value = new std::string(*other.m_data.string_value);
            } else {
                m_data = other.m_data;
            }
        }
        return *this;
    }

    InkType& operator=(InkType&& other) noexcept {
        if (this != &other) {
            cleanup();
            m_type = other.m_type;
            m_data = other.m_data;
            other.m_type = InkTypeId::Invalid;
            other.m_data.u64_value = 0;
        }
        return *this;
    }

    InkTypeId getType() const { return m_type; }
    bool isValid() const { return m_type != InkTypeId::Invalid; }

    Variant toVariant() const {
        switch (m_type) {
        case InkTypeId::I8:     return static_cast<i8>(m_data.i64_value);
        case InkTypeId::I16:    return static_cast<i16>(m_data.i64_value);
        case InkTypeId::I32:    return static_cast<i32>(m_data.i64_value);
        case InkTypeId::I64:    return m_data.i64_value;
        case InkTypeId::U8:     return static_cast<u8>(m_data.u64_value);
        case InkTypeId::U16:    return static_cast<u16>(m_data.u64_value);
        case InkTypeId::U32:    return static_cast<u32>(m_data.u64_value);
        case InkTypeId::U64:    return m_data.u64_value;
        case InkTypeId::F32:    return m_data.f32_value;
        case InkTypeId::F64:    return m_data.f64_value;
        case InkTypeId::Bool:   return m_data.bool_value;
        case InkTypeId::Char:   return m_data.char_value;
        case InkTypeId::Handle: return m_data.handle_value;
        case InkTypeId::String: return m_data.string_value ? *m_data.string_value : std::string();
        case InkTypeId::Invalid:
        default:                return std::monostate{};
        }
    }

private:
    union Data {
        i64 i64_value;
        u64 u64_value;
        f32 f32_value;
        f64 f64_value;
        bool bool_value;
        char char_value;
        ink_h handle_value;
        std::string* string_value;
    } m_data;

    InkTypeId m_type;

    void cleanup() {
        if (m_type == InkTypeId::String && m_data.string_value) {
            delete m_data.string_value;
            m_data.string_value = nullptr;
        }
    }
};

}

#endif
