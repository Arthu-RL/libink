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
    WString();                          // Default constructor
    WString(const char *s);             // C-string constructor
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

    // Utility methods
    WString to_lower() const noexcept;
    std::string toStdString() const noexcept;

    // Accessors
    size_t length() const noexcept;
    size_t capacity() const noexcept;
    const char* c_str() const noexcept;
    char* data() noexcept;
    const char* data() const noexcept;
    bool empty() const noexcept;

    // Display function (for debugging)
    void display() const;

private:
    static constexpr size_t SSO_SIZE = 64; // Small string optimization buffer size

    // Internal structure with union for Small String Optimization (SSO)
    union Data {
        struct {
            char* ptr;         // Pointer to heap allocated memory
            size_t size;       // String length
            size_t capacity;   // Allocated capacity
        } heap;

        struct {
            char buffer[SSO_SIZE]; // Small string buffer
            unsigned char size;    // String length (restricted by SSO_SIZE)
        } stack;
    };

    Data _data;
    bool _is_small;  // Flag to indicate if we're using stack storage

    // Private utility functions
    void _allocate(size_t capacity);
    void _deallocate();
    bool _is_using_sso() const noexcept;
    void _set_size(size_t size) noexcept;
    void _copy_from(const WString& other);
};

}

#endif // WSTRING_H
