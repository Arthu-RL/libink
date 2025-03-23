#include "../include/ink/WString.h"
#include <iostream>
#include <cstring>
#include <cctype>

// Default constructor
WString::WString() : _is_small(true) {
    _data.stack.buffer[0] = '\0';
    _data.stack.size = 0;
}

// C-string constructor
WString::WString(const char *s) : _is_small(true) {
    if (s == nullptr) {
        _data.stack.buffer[0] = '\0';
        _data.stack.size = 0;
        return;
    }

    size_t len = std::strlen(s);

    if (len < SSO_SIZE) {
        // Use small string optimization
        std::memcpy(_data.stack.buffer, s, len + 1);
        _data.stack.size = static_cast<unsigned char>(len);
    } else {
        // Use heap allocation
        _is_small = false;
        _data.heap.size = len;
        _data.heap.capacity = len + 1;
        _data.heap.ptr = new char[len + 1];
        std::memcpy(_data.heap.ptr, s, len + 1);
    }
}

// Copy constructor
WString::WString(const WString &src) {
    _copy_from(src);
}

// Move constructor
WString::WString(WString &&src) noexcept : _is_small(src._is_small) {
    if (src._is_small) {
        // Copy small string data
        std::memcpy(_data.stack.buffer, src._data.stack.buffer, SSO_SIZE);
        _data.stack.size = src._data.stack.size;
    } else {
        // Move heap data
        _data.heap.ptr = src._data.heap.ptr;
        _data.heap.size = src._data.heap.size;
        _data.heap.capacity = src._data.heap.capacity;

        // Reset source
        src._data.heap.ptr = nullptr;
        src._data.heap.size = 0;
        src._data.heap.capacity = 0;
        src._is_small = true;
    }
}

// Destructor
WString::~WString() {
    _deallocate();
}

// C-string assignment
WString &WString::operator=(const char* str) {
    if (str == nullptr) {
        _deallocate();
        _is_small = true;
        _data.stack.buffer[0] = '\0';
        _data.stack.size = 0;
        return *this;
    }

    size_t len = std::strlen(str);

    // First deallocate if we have heap memory
    if (!_is_small) {
        delete[] _data.heap.ptr;
    }

    if (len < SSO_SIZE) {
        // Use small string optimization
        _is_small = true;
        std::memcpy(_data.stack.buffer, str, len + 1);
        _data.stack.size = static_cast<unsigned char>(len);
    } else {
        // Use heap allocation
        _is_small = false;
        _data.heap.size = len;
        _data.heap.capacity = len + 1;
        _data.heap.ptr = new char[len + 1];
        std::memcpy(_data.heap.ptr, str, len + 1);
    }

    return *this;
}

// Copy assignment
WString &WString::operator=(const WString &src) {
    if (this == &src) return *this;

    _deallocate();
    _copy_from(src);
    return *this;
}

// Move assignment
WString &WString::operator=(WString &&src) noexcept {
    if (this == &src) return *this;

    _deallocate();

    _is_small = src._is_small;
    if (src._is_small) {
        // Copy small string data
        std::memcpy(_data.stack.buffer, src._data.stack.buffer, SSO_SIZE);
        _data.stack.size = src._data.stack.size;
    } else {
        // Move heap data
        _data.heap.ptr = src._data.heap.ptr;
        _data.heap.size = src._data.heap.size;
        _data.heap.capacity = src._data.heap.capacity;

        // Reset source
        src._data.heap.ptr = nullptr;
        src._data.heap.size = 0;
        src._data.heap.capacity = 0;
        src._is_small = true;
    }

    return *this;
}

// Equality comparison
bool WString::operator==(const WString &rhs) const {
    if (length() != rhs.length()) return false;

    if (_is_small && rhs._is_small) {
        return std::strcmp(_data.stack.buffer, rhs._data.stack.buffer) == 0;
    } else if (!_is_small && !rhs._is_small) {
        return std::strcmp(_data.heap.ptr, rhs._data.heap.ptr) == 0;
    } else if (_is_small) {
        return std::strcmp(_data.stack.buffer, rhs._data.heap.ptr) == 0;
    } else {
        return std::strcmp(_data.heap.ptr, rhs._data.stack.buffer) == 0;
    }
}

// Inequality comparison
bool WString::operator!=(const WString &rhs) const {
    return !(*this == rhs);
}

// String concatenation
WString WString::operator+(const WString &rhs) const {
    size_t left_len = length();
    size_t right_len = rhs.length();
    size_t total_len = left_len + right_len;

    WString result;

    if (total_len < SSO_SIZE) {
        // Result fits in small string
        result._is_small = true;

        // Copy left part
        if (_is_small) {
            std::memcpy(result._data.stack.buffer, _data.stack.buffer, left_len);
        } else {
            std::memcpy(result._data.stack.buffer, _data.heap.ptr, left_len);
        }

        // Copy right part
        if (rhs._is_small) {
            std::memcpy(result._data.stack.buffer + left_len, rhs._data.stack.buffer, right_len + 1);
        } else {
            std::memcpy(result._data.stack.buffer + left_len, rhs._data.heap.ptr, right_len + 1);
        }

        result._data.stack.size = static_cast<unsigned char>(total_len);
    } else {
        // Result needs heap allocation
        result._is_small = false;
        result._data.heap.size = total_len;
        result._data.heap.capacity = total_len + 1;
        result._data.heap.ptr = new char[total_len + 1];

        // Copy left part
        if (_is_small) {
            std::memcpy(result._data.heap.ptr, _data.stack.buffer, left_len);
        } else {
            std::memcpy(result._data.heap.ptr, _data.heap.ptr, left_len);
        }

        // Copy right part
        if (rhs._is_small) {
            std::memcpy(result._data.heap.ptr + left_len, rhs._data.stack.buffer, right_len + 1);
        } else {
            std::memcpy(result._data.heap.ptr + left_len, rhs._data.heap.ptr, right_len + 1);
        }
    }

    return result;
}

// Compound addition assignment
WString& WString::operator+=(const WString &rhs) {
    size_t left_len = length();
    size_t right_len = rhs.length();
    size_t total_len = left_len + right_len;

    if (_is_small && total_len < SSO_SIZE) {
        // Current is small and result fits in small string
        if (rhs._is_small) {
            std::memcpy(_data.stack.buffer + left_len, rhs._data.stack.buffer, right_len + 1);
        } else {
            std::memcpy(_data.stack.buffer + left_len, rhs._data.heap.ptr, right_len + 1);
        }
        _data.stack.size = static_cast<unsigned char>(total_len);
    } else {
        // Need heap allocation
        char* new_buffer = new char[total_len + 1];

        // Copy this string
        if (_is_small) {
            std::memcpy(new_buffer, _data.stack.buffer, left_len);
        } else {
            std::memcpy(new_buffer, _data.heap.ptr, left_len);
        }

        // Copy rhs string
        if (rhs._is_small) {
            std::memcpy(new_buffer + left_len, rhs._data.stack.buffer, right_len + 1);
        } else {
            std::memcpy(new_buffer + left_len, rhs._data.heap.ptr, right_len + 1);
        }

        // Deallocate old buffer if needed
        if (!_is_small) {
            delete[] _data.heap.ptr;
        }

        // Update to heap storage
        _is_small = false;
        _data.heap.ptr = new_buffer;
        _data.heap.size = total_len;
        _data.heap.capacity = total_len + 1;
    }

    return *this;
}

// Create lowercase version of string
WString WString::to_lower() const noexcept {
    WString result(*this);

    if (result._is_small) {
        for (size_t i = 0; i < result._data.stack.size; ++i) {
            result._data.stack.buffer[i] = std::tolower(result._data.stack.buffer[i]);
        }
    } else {
        for (size_t i = 0; i < result._data.heap.size; ++i) {
            result._data.heap.ptr[i] = std::tolower(result._data.heap.ptr[i]);
        }
    }

    return result;
}

// Get string length
size_t WString::length() const noexcept {
    return _is_small ? _data.stack.size : _data.heap.size;
}

// Get string capacity
size_t WString::capacity() const noexcept {
    return _is_small ? SSO_SIZE - 1 : _data.heap.capacity - 1;
}

// Get C-string representation
const char* WString::c_str() const noexcept {
    return _is_small ? _data.stack.buffer : _data.heap.ptr;
}

// Get mutable data pointer
char* WString::data() noexcept {
    return _is_small ? _data.stack.buffer : _data.heap.ptr;
}

// Get const data pointer
const char* WString::data() const noexcept {
    return _is_small ? _data.stack.buffer : _data.heap.ptr;
}

// Check if string is empty
bool WString::empty() const noexcept {
    return length() == 0;
}

// Display string info
void WString::display() const {
    std::cout << (_is_small ? _data.stack.buffer : _data.heap.ptr)
    << ": length=" << length()
    << ", capacity=" << capacity()
    << ", using " << (_is_small ? "stack" : "heap")
    << std::endl;
}

// Stream output operator
std::ostream &operator<<(std::ostream &os, const WString &obj) {
    os << (obj._is_small ? obj._data.stack.buffer : obj._data.heap.ptr);
    return os;
}

// Private helper to allocate memory
void WString::_allocate(size_t capacity) {
    if (capacity < SSO_SIZE) {
        _is_small = true;
    } else {
        _is_small = false;
        _data.heap.capacity = capacity;
        _data.heap.ptr = new char[capacity];
        _data.heap.size = 0;
        _data.heap.ptr[0] = '\0';
    }
}

// Private helper to deallocate memory
void WString::_deallocate() {
    if (!_is_small && _data.heap.ptr != nullptr) {
        delete[] _data.heap.ptr;
        _data.heap.ptr = nullptr;
    }
}

// Check if using small string optimization
bool WString::_is_using_sso() const noexcept {
    return _is_small;
}

// Set string size
void WString::_set_size(size_t size) noexcept {
    if (_is_small) {
        _data.stack.size = static_cast<unsigned char>(size);
    } else {
        _data.heap.size = size;
    }
}

// Copy from another WString
void WString::_copy_from(const WString& other) {
    _is_small = other._is_small;

    if (other._is_small) {
        std::memcpy(_data.stack.buffer, other._data.stack.buffer, SSO_SIZE);
        _data.stack.size = other._data.stack.size;
    } else {
        _data.heap.size = other._data.heap.size;
        _data.heap.capacity = other._data.heap.capacity;
        _data.heap.ptr = new char[other._data.heap.capacity];
        std::memcpy(_data.heap.ptr, other._data.heap.ptr, other._data.heap.capacity);
    }
}
