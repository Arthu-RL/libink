#ifndef INKQUEUE_H
#define INKQUEUE_H

#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <optional>

#include "ink/ink_base.hpp"

namespace ink {

template<typename T>
class INK_API InkQueue {
private:
    mutable std::mutex mutex_;
    std::queue<T> data_queue_;
    std::condition_variable data_cond_;
    std::atomic<bool> done_;

public:
    InkQueue() : done_(false) {}

    InkQueue(const InkQueue&) = delete;
    InkQueue& operator=(const InkQueue&) = delete;

    ~InkQueue() {
        shutdown();
    }

    void push(T new_value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_queue_.push(std::move(new_value));
        data_cond_.notify_one();
    }

    template<typename Iterator>
    void push_bulk(Iterator begin, Iterator end) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = begin; it != end; ++it) {
            data_queue_.push(std::move(*it));
        }
        size_t count = std::distance(begin, end);
        for (size_t i = 0; i < count; ++i) {
            data_cond_.notify_one();
        }
    }

    bool wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        data_cond_.wait(lock, [this] {
            return !data_queue_.empty() || done_;
        });

        if (data_queue_.empty()) {
            return false;
        }

        value = std::move(data_queue_.front());
        data_queue_.pop();
        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_queue_.empty()) {
            return false;
        }

        value = std::move(data_queue_.front());
        data_queue_.pop();
        return true;
    }

    std::optional<T> pop_front() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (data_queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(data_queue_.front());
        data_queue_.pop();
        return value;
    }

    template<typename Rep, typename Period>
    bool try_pop_for(T& value, const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!data_cond_.wait_for(lock, timeout, [this] {
                return !data_queue_.empty() || done_;
            })) {
            return false;
        }

        if (data_queue_.empty()) {
            return false;
        }

        value = std::move(data_queue_.front());
        data_queue_.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_queue_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_queue_.size();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            done_ = true;
        }
        data_cond_.notify_all();
    }

    bool is_shutdown() const {
        return done_;
    }
};

}

#endif // INKQUEUE_H
