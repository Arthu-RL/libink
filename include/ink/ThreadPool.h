#ifndef THREADPOOL_H
#define THREADPOOL_H

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <future>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <tbb/concurrent_queue.h>

#include "ink/ink_base.hpp"

namespace ink {

class INK_API ThreadPool {
public:
    ThreadPool(ink_size max_workers);
    ~ThreadPool();

    template <typename Function, typename... Args>
    std::future<std::invoke_result_t<Function, Args...>> submit(Function&& f, Args&&... args)
    {
        using ReturnType = std::invoke_result_t<Function, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>
        (
            [fn = std::forward<Function>(f), tpl = std::make_tuple(std::forward<Args>(args)...)]() mutable -> ReturnType
            {
                return std::apply(std::move(fn), std::move(tpl));
            }
        );

        std::future<ReturnType> res = task->get_future();

        if (_stop.load()) throw std::runtime_error("ThreadPool is stopped");
        _tasks.push([task]() { (*task)(); });

        _condition.notify_one();
        return res;
    }

    void wait();

private:
    std::vector<std::thread> _workers;
    std::atomic<int> _active_workers;
    tbb::concurrent_queue<std::function<void()>> _tasks;

    std::mutex _tpMutex;
    std::condition_variable _condition;
    std::atomic<ink_bool> _stop;
};

}

#endif
