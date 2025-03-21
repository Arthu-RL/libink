#include "../include/ink/ThreadPool.h"

namespace ink {

ThreadPool::ThreadPool(ink_size max_workers) :
    _stop(false), _active_workers(0)
{
    for (ink_size i = 0; i < max_workers; ++i)
    {
        _workers.emplace_back([this] {
            while (true)
            {
                std::function<void()> _task;
                {
                    std::unique_lock<std::mutex> lock(_queueMutex);
                    _condition.wait(lock, [this]{ return _stop || !_tasks.empty(); });

                    if (_stop && _tasks.empty()) return;

                    _task = std::move(_tasks.front());
                    _tasks.pop();
                    _active_workers++;
                }

                _task();

                _active_workers--;
                _condition.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _stop = true;
    }
    _condition.notify_all();
    for (std::thread& worker : _workers) {
        worker.join();
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(_queueMutex);
    _condition.wait(lock, [this] { return _tasks.empty() && (_active_workers == 0); });
}

}
