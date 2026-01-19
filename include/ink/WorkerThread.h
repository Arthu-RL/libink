#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Assuming ink_base.hpp defines INK_API
#include "ink_base.hpp"

namespace ink {

class INK_API WorkerThread
{
public:
    enum Policy
    {
        WaitTimeout = 0,      // Stop immediately (don't wait for current process logic to finish) - no thread cancellation
        WaitProcessFinish = 1 // Allow the current 'process()' call to complete before joining
    };

    WorkerThread(Policy policy, size_t timeoutSecs);
    virtual ~WorkerThread();

    typedef std::function<void()> WTCallback;

    void start();
    void stop();

    void wake();

    void setOnStartAction(WTCallback onStartCallback) noexcept;
    void setOnDestructionAction(WTCallback onDestructionCallback) noexcept;

    bool isRunning() const { return _isRunning; }
    bool isProcessing() const { return _isProcessing; }

protected:
    virtual void process() = 0;

private:
    void _process();

    std::atomic<bool> _isRunning;
    std::atomic<bool> _isProcessing;
    std::atomic<bool> _requestProcessing;

    Policy _policy;
    size_t _timeoutMs;

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;

    WTCallback _onStartCallback;
    WTCallback _onDestructionCallback;
};

}

#endif // WORKERTHREAD_H
