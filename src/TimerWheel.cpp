#include "../include/ink/TimerWheel.h"

#include "../include/ink/utils.h"

namespace ink {

TimerWheel::TimerWheel(u32 ticksToLive, u32 tickIntervalMs) :
    _ticksToLive(ticksToLive),
    _wheel(ticksToLive, nullptr),
    _currentSlot(0),
    _tickMs(tickIntervalMs),
    _lastTickMs(ink::utils::nowMillis())
{
    // Empty
}

void TimerWheel::update(TimerNode* node)
{
    // Unlink from old position (if any)
    unlink(node);

    // Calculate new slot (Current + Timeout)
    // If current is 5 and timeout is 60, newSlot is (5 + 60 + 1) % 60 = 6.
    // This means it will expire exactly one full revolution
    // The +1 ensures we don't land in the "current" slot so it willl not expire early
    u32 newSlot = (_currentSlot + _ticksToLive + 1) % _wheel.size();

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

u64 TimerWheel::timeToNextTickMillis() const
{
    u64 now = ink::utils::nowMillis();
    u64 elapsed = now - _lastTickMs;

    if (elapsed >= _tickMs)
        return 0;

    return static_cast<u64>(_tickMs - elapsed);
}


}

