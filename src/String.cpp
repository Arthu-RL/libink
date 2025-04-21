#include "../include/ink/String.h"
#include <iostream>
#include <cstring>
#include <cctype>

namespace ink {

String::String(size_t small_buffer_size)
{
    _data = static_cast<Data*>(::operator new(_calculate_data_size(small_buffer_size)));
    _data->is_small = true;
    _data->sso_capacity = small_buffer_size;
    _data->stack.buffer[0] = '\0';
}

String::String(const char *s, size_t small_buffer_size)
{
    _data = static_cast<Data*>(::operator new(_calculate_data_size(small_buffer_size)));
    _data->sso_capacity = small_buffer_size;

    if (s == nullptr)
    {
        _data->is_small = true;
        _data->stack.buffer[0] = '\0';
        return;
    }

    size_t len = std::strlen(s);
    if (len < small_buffer_size)
    {
        _data->is_small = true;
        std::memcpy(_data->stack.buffer, s, len + 1);
    }
    else
    {
        _data->is_small = false;
        _data->heap.size = len;
        _data->heap.capacity = len + 1;
        _data->heap.ptr = new char[len + 1];
        std::memcpy(_data->heap.ptr, s, len + 1);
    }
}

String::String(const String &src)
{
    _copy_from(src);
}

String::String(String &&src) noexcept
{
    _data = src._data;
    src._data = nullptr;
}

String::~String()
{
    _deallocate();
}

String &String::operator=(const char* str)
{
    if (str == nullptr)
    {
        if (!_data->is_small && _data->heap.ptr != nullptr)
        {
            delete[] _data->heap.ptr;
        }
        _data->is_small = true;
        _data->stack.buffer[0] = '\0';
        return *this;
    }

    size_t len = std::strlen(str);

    if (_data->is_small)
    {
        if (len < _data->sso_capacity)
        {
            std::memcpy(_data->stack.buffer, str, len + 1);
        }
        else
        {
            _data->is_small = false;
            _data->heap.size = len;
            _data->heap.capacity = len + 1;
            _data->heap.ptr = new char[len + 1];
            std::memcpy(_data->heap.ptr, str, len + 1);
        }
    }
    else
    {
        if (len < _data->sso_capacity)
        {
            if (_data->heap.ptr != nullptr)
            {
                delete[] _data->heap.ptr;
            }
            _data->is_small = true;
            std::memcpy(_data->stack.buffer, str, len + 1);
        }
        else if (len + 1 <= _data->heap.capacity)
        {
            std::memcpy(_data->heap.ptr, str, len + 1);
            _data->heap.size = len;
        }
        else
        {
            if (_data->heap.ptr != nullptr)
            {
                delete[] _data->heap.ptr;
            }
            _data->heap.size = len;
            _data->heap.capacity = len + 1;
            _data->heap.ptr = new char[len + 1];
            std::memcpy(_data->heap.ptr, str, len + 1);
        }
    }
    return *this;
}

String &String::operator=(const String &src)
{
    if (this == &src) return *this;
    _deallocate();
    _copy_from(src);
    return *this;
}

String &String::operator=(String &&src) noexcept
{
    if (this == &src) return *this;
    _deallocate();
    _data = src._data;
    src._data = nullptr;
    return *this;
}

bool String::operator==(const String &rhs) const
{
    if (length() != rhs.length()) return false;
    return std::strcmp(c_str(), rhs.c_str()) == 0;
}

bool String::operator!=(const String &rhs) const
{
    return !(*this == rhs);
}

String String::operator+(const String &rhs) const
{
    String result(*this);
    result += rhs;
    return result;
}

String& String::operator+=(const String &rhs)
{
    size_t left_len = length();
    size_t right_len = rhs.length();
    size_t total_len = left_len + right_len;

    if (_data->is_small)
    {
        if (total_len < _data->sso_capacity)
        {
            std::memcpy(_data->stack.buffer + left_len, rhs.c_str(), right_len + 1);
        }
        else
        {
            char* new_buffer = new char[total_len + 1];
            std::memcpy(new_buffer, _data->stack.buffer, left_len);
            std::memcpy(new_buffer + left_len, rhs.c_str(), right_len + 1);

            _data->is_small = false;
            _data->heap.ptr = new_buffer;
            _data->heap.size = total_len;
            _data->heap.capacity = total_len + 1;
        }
    }
    else
    {
        if (total_len + 1 <= _data->heap.capacity)
        {
            std::memcpy(_data->heap.ptr + left_len, rhs.c_str(), right_len + 1);
            _data->heap.size = total_len;
        }
        else
        {
            size_t new_capacity = std::max(total_len + 1, _data->heap.capacity * 2);
            char* new_buffer = new char[new_capacity];
            std::memcpy(new_buffer, _data->heap.ptr, left_len);
            std::memcpy(new_buffer + left_len, rhs.c_str(), right_len + 1);

            delete[] _data->heap.ptr;
            _data->heap.ptr = new_buffer;
            _data->heap.size = total_len;
            _data->heap.capacity = new_capacity;
        }
    }

    return *this;
}

String String::to_lower() const noexcept
{
    String result(*this);
    char* data_ptr = result.data();

    for (size_t i = 0; i < result.length(); ++i)
    {
        data_ptr[i] = std::tolower(static_cast<unsigned char>(data_ptr[i]));
    }

    return result;
}

std::string String::toStdString() const noexcept
{
    return std::string(c_str(), length());
}

size_t String::length() const noexcept
{
    if (_data->is_small)
    {
        return std::strlen(_data->stack.buffer);
    }
    else
    {
        return _data->heap.size;
    }
}

size_t String::capacity() const noexcept
{
    if (_data->is_small)
    {
        return _data->sso_capacity - 1;
    }
    else
    {
        return _data->heap.capacity - 1;
    }
}

const char* String::c_str() const noexcept
{
    if (_data->is_small)
    {
        return _data->stack.buffer;
    }
    else
    {
        return _data->heap.ptr;
    }
}

char* String::data() noexcept
{
    if (_data->is_small)
    {
        return _data->stack.buffer;
    }
    else
    {
        return _data->heap.ptr;
    }
}

bool String::empty() const noexcept
{
    return length() == 0;
}

void String::display() const
{
    std::cout << c_str()
    << ": length=" << length()
    << ", capacity=" << capacity()
    << ", using " << (_data->is_small ? "stack" : "heap")
    << ", sso_capacity=" << _data->sso_capacity
    << std::endl;
}

std::ostream &operator<<(std::ostream &os, const String &obj)
{
    os << obj.c_str();
    return os;
}

void String::_deallocate()
{
    if (_data)
    {
        if (!_data->is_small && _data->heap.ptr != nullptr)
        {
            delete[] _data->heap.ptr;
        }
        ::operator delete(_data);
        _data = nullptr;
    }
}

bool String::_is_using_sso() const noexcept
{
    return _data->is_small;
}

void String::_set_size(size_t size) noexcept
{
    if (_data->is_small)
    {
        _data->stack.buffer[size] = '\0';
    }
    else
    {
        _data->heap.size = size;
        if (_data->heap.ptr)
        {
            _data->heap.ptr[size] = '\0';
        }
    }
}

void String::_copy_from(const String& other)
{
    _data = static_cast<Data*>(::operator new(_calculate_data_size(other._data->sso_capacity)));
    _data->sso_capacity = other._data->sso_capacity;
    _data->is_small = other._data->is_small;

    if (other._data->is_small)
    {
        std::memcpy(_data->stack.buffer, other._data->stack.buffer, other.length() + 1);
    }
    else
    {
        _data->heap.size = other._data->heap.size;
        _data->heap.capacity = other._data->heap.capacity;
        _data->heap.ptr = new char[_data->heap.capacity];
        std::memcpy(_data->heap.ptr, other._data->heap.ptr, _data->heap.size + 1);
    }
}

size_t String::_calculate_data_size(size_t sso_capacity)
{
    return sizeof(Data) + sso_capacity;
}

}
