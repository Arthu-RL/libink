#include "../include/ink/TimerWheel.h"

#include "../include/ink/utils.h"

namespace ink {

TimerWheel::TimerWheel(u32 ticksToLive, u32 tickIntervalMs) :
    _ticksToLive(ticksToLive),
    _currentSlot(0),
    _tickMs(tickIntervalMs),
    _lastTickMs(ink::utils::nowMillis())
{
    u32 requiredSize = _ticksToLive + 1;
    // Get power of 2 number next tp requiredSize
    u32 power = 1;
    while (power < requiredSize)
    {
        power <<= 1;
    }

    _wheel.assign(power, nullptr);
    _wheelMask = power - 1;
}

void TimerWheel::update(TimerNode* node)
{
    // Unlink from old position (if any)
    unlink(node);

    // Calculate new slot (Current + Timeout)
    // If current is 5 and timeout is 60, newSlot is (5 + 60) % 60 = 5.
    // This means it will expire exactly one full revolution
    u32 newSlot = (_currentSlot + _ticksToLive) & _wheelMask;

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

    _currentSlot = (_currentSlot + 1) & _wheelMask;
    _lastTickMs += _tickMs;

    return expiredList;
}

u64 TimerWheel::timeToNextTickMillis(const u64& nowMs) const
{
    u64 elapsed = nowMs - _lastTickMs;

    if (elapsed >= _tickMs)
        return 0;

    return static_cast<u64>(_tickMs - elapsed);
}


}

