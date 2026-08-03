#include "../include/ink/ThreadPool.h"

#include <stdexcept>

namespace ink {

ThreadPool::ThreadPool(size_t max_workers) :
    _stop(false)
{
    if (max_workers == 0)
        throw std::invalid_argument("ThreadPool requires at least one worker");

    _workers.reserve(max_workers);

    try 
    {
        for (size_t i = 0; i < max_workers; ++i)
        {
            _workers.emplace_back([this] {
                while (true)
                {
                    ink::move_only_function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(_tpMutex);
                        _condition.wait(lock, [this]{ return _stop || !_tasks.empty(); });

                        if (_stop && _tasks.empty()) return;

                        task = std::move(_tasks.front());
                        _tasks.pop();
                    }

                    task();
                }
            });
        }
    } 
    catch (...) 
    {
        // std::thread construction failed partway through: stop and join
        // the workers already spawned before rethrowing, otherwise their
        // destructors would call std::terminate() on a still-joinable
        // thread while this object never finished constructing.
        {
            std::lock_guard<std::mutex> lock(_tpMutex);
            _stop = true;
        }
        _condition.notify_all();
        for (std::thread& worker : _workers) 
        {
            if (worker.joinable()) 
                worker.join();
        }
        throw;
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
