#include "../include/ink/TimerWheel.h"

#include "../include/ink/utils.h"

namespace ink {

TimerWheel::TimerWheel(u32 size) :
    _wheel(size, nullptr),
    _currentSlot(0),
    _tickMs(1000),
    _lastTickMs(ink::utils::nowMillis())
{
    // Empty
}

void TimerWheel::update(TimerNode* node)
{
    // Unlink from old position (if any)
    unlink(node);

    // Calculate new slot (Current + Timeout - 1)
    // If timeout is 10s, and we are at slot 5, it goes to slot 15.
    // We wrap around using modulo.
    u32 ticksToLive = _wheel.size() - 1;
    u32 newSlot = (_currentSlot + ticksToLive) % _wheel.size();

    // Link to new bucket
    node->slotIndex = newSlot;
    node->next = _wheel[newSlot];
    node->prev = nullptr;

    if (_wheel[newSlot])
    {
        _wheel[newSlot]->prev = node;
    }
    _wheel[newSlot] = node;
}

void TimerWheel::unlink(TimerNode* node)
{
    if (!node->prev && !node->next && _wheel[node->slotIndex] != node)
    {
        return; // Not linked
    }

    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;

    // If this was the head, update the head
    if (_wheel[node->slotIndex] == node)
    {
        _wheel[node->slotIndex] = node->next;
    }

    node->prev = nullptr;
    node->next = nullptr;
}

TimerNode* TimerWheel::tick()
{
    // Get the bucket at the current hand
    TimerNode* expiredList = _wheel[_currentSlot];
    _wheel[_currentSlot] = nullptr;

    _currentSlot = (_currentSlot + 1) % _wheel.size();
    _lastTickMs += _tickMs;

    return expiredList;
}

i32 TimerWheel::timeToNextTickMillis() const
{
    uint64_t now = ink::utils::nowMillis();
    uint64_t elapsed = now - _lastTickMs;

    if (elapsed >= _tickMs)
        return 0;

    return static_cast<i32>(_tickMs - elapsed);
}


}

