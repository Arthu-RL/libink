#include "../include/ink/RingBuffer.h"

namespace ink {

RingBuffer::RingBuffer(size_t capacity) :
    _buffer(new char[capacity]),
    _capacity(capacity),
    _readPos(0),
    _writePos(0),
    _size(0)
{
    // Empty
}

RingBuffer::~RingBuffer()
{
    delete[] _buffer;
}

size_t RingBuffer::read(char* dest, size_t maxLen)
{
    if (maxLen == 0 || _size == 0)
        return 0;

    const size_t toRead = std::min(maxLen, _size);

    const size_t tail = _capacity - _readPos;
    const size_t first = std::min(toRead, tail);

    // First chunk
    memcpy(dest, _buffer + _readPos, first);

    // Wrap-around chunk
    if (toRead > first)
        memcpy(dest + first, _buffer, toRead - first);

    _readPos += toRead;
    if (_readPos >= _capacity)
        _readPos -= _capacity;

    _size -= toRead;
    return toRead;
}

size_t RingBuffer::write(const char* data, size_t len)
{
    if (len == 0 || _size == _capacity)
        return 0;

    const size_t toWrite = std::min(len, _capacity - _size);

    const size_t tail = _capacity - _writePos;
    const size_t first = std::min(toWrite, tail);

    memcpy(_buffer + _writePos, data, first);

     // Wrap-around chunk
    if (toWrite > first)
        memcpy(_buffer, data + first, toWrite - first);

    _writePos += toWrite;
    if (_writePos >= _capacity)
        _writePos -= _capacity;

    _size += toWrite;
    return toWrite;
}

size_t RingBuffer::write(std::string_view sv)
{
    return write(sv.data(), sv.size());
}

size_t RingBuffer::write(const std::string& s)
{
    return write(s.data(), s.size());
}

const char* RingBuffer::getReadBuffer(size_t& availableData) const
{
    if (_size == 0) {
        availableData = 0;
        return nullptr;
    }

    if (_readPos < _writePos)
        availableData = _writePos - _readPos; // Simple case: read position before write position
    else
        availableData = _capacity - _readPos; // Complex case: read position after write position (wrap-around)

    return _buffer + _readPos;
}

char* RingBuffer::getWriteBuffer(size_t& availableSpace)
{
    if (_size == _capacity) {
        availableSpace = 0;
        return nullptr;
    }

    if (_writePos >= _readPos)
        availableSpace = _capacity - _writePos; // Write position at or after read position
    else
        availableSpace = _readPos - _writePos; // Write position before read position

    return _buffer + _writePos;
}

void RingBuffer::advanceReadPos(size_t len)
{
    size_t bytesToAdvance = std::min(len, _size);
    _readPos = (_readPos + bytesToAdvance) % _capacity;
    _size -= bytesToAdvance;
}

// Advance write position after writing data
void RingBuffer::advanceWritePos(size_t len)
{
    size_t bytesToAdvance = std::min(len, _capacity - _size);
    _writePos = (_writePos + bytesToAdvance) % _capacity;
    _size += bytesToAdvance;
}

// Clear buffer
void RingBuffer::clear()
{
    _readPos = 0;
    _writePos = 0;
    _size = 0;
}

}
