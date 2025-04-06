#ifndef WSTRING_H
#define WSTRING_H

#pragma once

#include <ostream>
#include <cstddef>
#include <cstring>

namespace ink {

class WString {
    friend std::ostream &operator<<(std::ostream &os, const WString &obj);

public:
    // Constructors and destructor
    WString(size_t small_buffer_size = 64);
    WString(const char *s, size_t small_buffer_size = 64);
    WString(const WString &src);        // Copy constructor
    WString(WString &&src) noexcept;    // Move constructor
    ~WString();                         // Destructor

    // Assignment operators
    WString &operator=(const char* str);      // C-string assignment
    WString &operator=(const WString &src);   // Copy assignment
    WString &operator=(WString &&src) noexcept; // Move assignment

    // Comparison operators
    bool operator==(const WString &rhs) const;
    bool operator!=(const WString &rhs) const;

    // Concatenation operators
    WString operator+(const WString &rhs) const;
    WString& operator+=(const WString &rhs);

    WString to_lower() const noexcept;
    // Convert to std::string. Obs: not good for high performance requirements
    std::string toStdString() const noexcept;

    size_t length() const noexcept;
    // Get string capacity
    size_t capacity() const noexcept;
    // Get C-string representation. Obs: good for high performance usage
    const char* c_str() const noexcept;
    // Get mutable data pointer
    char* data() noexcept;
    bool empty() const noexcept;

    // Display function (for debugging)
    void display() const;

private:
    // Internal structure with union for Small String Optimization (SSO)
    union Data {
        struct {
            char* ptr;         // Pointer to heap allocated memory
            size_t size;       // String length
            size_t capacity;   // Allocated capacity
        } heap;

        struct {
            char buffer[1]; // Small string buffer
            unsigned char size;    // String length (restricted by SSO_SIZE)
        } stack;

        bool is_small;
        size_t sso_capacity;
    };

    Data* _data;

    // Private utility functions
    void _allocate(size_t capacity);
    void _deallocate();
    bool _is_using_sso() const noexcept;
    void _set_size(size_t size) noexcept;
    void _copy_from(const WString& other);
    static size_t _calculate_data_size(size_t sso_capacity);
};

}

namespace std {
template<>
struct hash<ink::WString> {
    size_t operator()(const ink::WString& str) const noexcept {
        // Use FNV-1a hash algorithm - fast and good distribution
        size_t hash = 14695981039346656037ULL; // FNV offset basis
        const char* data = str.c_str();
        for (size_t i = 0; i < str.length(); ++i) {
            hash ^= static_cast<size_t>(data[i]);
            hash *= 1099511628211ULL; // FNV prime
        }
        return hash;
    }
};
}

#endif // WSTRING_H
