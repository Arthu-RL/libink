#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#pragma once

#include <math.h>

#include "ink/ink_base.hpp"

namespace ink {

class INK_API RingBuffer {
public:
    explicit RingBuffer(size_t capacity = 8192);

    ~RingBuffer();

    // Non-copyable
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // Read data from the buffer
    size_t read(char* dest, size_t maxLen);
    // Write data to the buffer
    size_t write(const char* data, size_t len);

    // Get a contiguous read buffer (for zero-copy operations)
    const char* getReadBuffer(size_t& availableData) const;
    // Get a contiguous write buffer (for zero-copy operations)
    char* getWriteBuffer(size_t& availableSpace);
    // Advance read position after reading data
    void advanceReadPos(size_t len);
    // Advance write position after writing data
    void advanceWritePos(size_t len);
    // Clear buffer
    void clear();

    // Utility methods
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    bool empty() const { return _size == 0; }
    bool full() const { return _size == _capacity; }

private:
    char* _buffer;
    size_t _capacity;
    size_t _readPos;
    size_t _writePos;
    size_t _size;
};

}

#endif // RINGBUFFER_H
