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

#include "ink/ink_base.hpp"

namespace ink {

class INK_API ThreadPool {
public:
    ThreadPool(size_t max_workers);
    ~ThreadPool();

    template <typename Function, typename... Args>
    std::future<std::invoke_result_t<Function, Args...>> submit(Function&& f, Args&&... args)
    {
        using ReturnType = std::invoke_result_t<Function, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [fn = std::forward<Function>(f), tpl = std::make_tuple(std::forward<Args>(args)...)]() mutable -> ReturnType
            {
                return std::apply(std::move(fn), std::move(tpl));
            }
        );

        std::future<ReturnType> res = task->get_future();

        {
            std::lock_guard<std::mutex> lock(_tpMutex);
            if (_stop) throw std::runtime_error("ThreadPool is stopped");

            _tasks.push([task]() { (*task)(); });
        }

        _condition.notify_one();
        return res;
    }

private:
    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;

    std::mutex _tpMutex;
    std::condition_variable _condition;
    bool _stop;
};

}

#endif
