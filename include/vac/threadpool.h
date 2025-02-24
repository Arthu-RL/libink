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

namespace vac {

class ThreadPool {
public:
    ThreadPool(size_t max_workers);
    ~ThreadPool();

    template <typename F, typename... Args>
    std::future<std::invoke_result_t<F, Args...>> submit(F&& f, Args&&... args)
    {
        using returnType = typename std::invoke_result<F, Args...>::type;
        auto task = std::make_shared<std::packaged_task<returnType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<returnType> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("ThreadPool is stopped");
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

}

#endif
