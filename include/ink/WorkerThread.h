#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "ink_base.hpp"

namespace ink {

enum Policy
{
    KillImediately = 0,
    WaitProcessFinish = 1,
    WaitTimeout = 2
};

class INK_API WorkerThread
{
public:
    WorkerThread();
    WorkerThread(Policy policy);
    WorkerThread(size_t timeoutSecs);
    WorkerThread(Policy policy, size_t timeoutSecs);
    virtual ~WorkerThread();

    void start();
    void stop();
    void setOnStartAction(std::function<void()> onStartCallback);
    void setOnDestructionAction(std::function<void()> onDestructionCallback);

    bool isRunning() const { return _isRunning; }

protected:
    virtual void process() = 0;

private:
    void _process();

    std::atomic<bool> _isRunning;
    std::atomic<bool> _isProcessing;
    Policy _policy;
    size_t _timeoutMs;

    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;

    std::function<void()> _onStartCallback;
    std::function<void()> _onDestructionCallback;
};

}

#endif // WORKERTHREAD_H
