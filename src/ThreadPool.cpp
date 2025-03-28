#include "../include/ink/ThreadPool.h"

namespace ink {

ThreadPool::ThreadPool(ink_size max_workers) :
    _stop(false)
{
    for (ink_size i = 0; i < max_workers; ++i)
    {
        _workers.emplace_back([this] {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(_tpMutex);
                    _condition.wait(lock, [this]{ return _stop || !_tasks.empty(); });

                    if (_stop && _tasks.empty()) return;

                    if (_tasks.empty()) continue;
                    task = std::move(_tasks.front());
                    _tasks.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(_tpMutex);
        _stop = true;
    }

    _condition.notify_all();

    for (std::thread& worker : _workers) {
        worker.join();
    }
}

}
