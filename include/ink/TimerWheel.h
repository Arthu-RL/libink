#ifndef TIMERWHEEL_H
#define TIMERWHEEL_H

#include <vector>
#include "ink/ink_base.hpp"

namespace ink {

struct TimerNode {
    TimerNode* prev = nullptr;
    TimerNode* next = nullptr;
    u32 slotIndex = 0;
};

class TimerWheel {
public:
    // Resolution: 1 tick per second (or 100ms)
    // Size: 60 slots (for 60 seconds timeout)
    TimerWheel(u32 size = 60);

    // O(1) - Add or Update session
    void update(TimerNode* node);

    // O(1) - Remove session (e.g., on explicit close)
    void unlink(TimerNode* node);

    // O(1) Batch - Process timeouts
    // Returns a list of expired nodes to be closed
    TimerNode* tick();

    template <typename Fn>
    void processExpired(Fn&& fn)
    {
        TimerNode* node = tick();

        while (node)
        {
            TimerNode* next = node->next;

            node->prev = nullptr;
            node->next = nullptr;

            fn(node);

            node = next;
        }
    }

    i32 timeToNextTickMillis() const;

private:
    std::vector<TimerNode*> _wheel;
    u32 _currentSlot;

    u32 _tickMs;
    u64 _lastTickMs;
};

}

#endif // LASTWISH_H
