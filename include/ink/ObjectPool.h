#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <vector>

#include "ink_base.hpp"

template<typename T, usize iSize>
class ObjectPool
{
public:
    ObjectPool() : _currentCapacity(iSize) {
        expand(_currentCapacity);
    }

    // avoid double-freeing the memory blocks
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ~ObjectPool() {
        for (void* block : _allBlocks) {
            ::operator delete[](block, std::align_val_t(alignof(T)));
        }
    }

    T* acquire() {
        if (_freeList.empty()) {
            _currentCapacity *= 2;
            expand(_currentCapacity);
        }
        T* obj = _freeList.back();
        _freeList.pop_back();
        return obj;
    }

    void release(T* obj) {
        _freeList.push_back(obj);
    }

private:
    void expand(usize count) {
        // Enforce the alignas(32) requirement when allocating the raw slab
        // Make a big block of T* type space
        T* block = static_cast<T*>(::operator new[](count * sizeof(T), std::align_val_t(alignof(T))));
        _allBlocks.push_back(block);

        // Push in reverse order so that 'acquire()' pops sequential addresses
        // Insert each T* space of the block
        for (isize i = count - 1; i >= 0; --i) {
            _freeList.push_back(&block[i]);
        }
    }

    std::vector<T*> _freeList;
    std::vector<void*> _allBlocks;
    usize _currentCapacity;
};

#endif // OBJECTPOOL_H
