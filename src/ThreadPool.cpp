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
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(_tpMutex);
                    _condition.wait(lock, [this]{ return _stop.load() || !_tasks.empty(); });

                    if (_stop.load() && _tasks.empty()) return;
                }

                if (_tasks.try_pop(task))
                {
                    _active_workers++;
                    task();
                    _active_workers--;
                }

                if (_tasks.empty() && _active_workers.load() == 0)
                    _condition.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    _stop.store(true);
    _condition.notify_all();

    for (std::thread& worker : _workers) {
        worker.join();
    }
}

void ThreadPool::wait()
{
    std::unique_lock<std::mutex> lock(_tpMutex);
    _condition.wait(lock, [this] {
        return _tasks.empty() && (_active_workers.load() == 0);
    });
}

}
