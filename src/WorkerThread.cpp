#include "../include/ink/WorkerThread.h"

#include <chrono>

namespace ink {

WorkerThread::WorkerThread(Policy policy, size_t timeoutSecs) :
    _isRunning(false),
    _isProcessing(false),
    _requestProcessing(false),
    _policy(policy),
    _timeoutMs(timeoutSecs * 1000),
    _onStartCallback(nullptr),
    _onDestructionCallback(nullptr)
{
}

WorkerThread::~WorkerThread()
{
    stop();
}

void WorkerThread::start()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_isRunning)
        return;

    _isRunning = true;
    _requestProcessing = false;

    if (_onStartCallback)
    {
        _onStartCallback();
    }

    _thread = std::thread(&WorkerThread::_process, this);
}

void WorkerThread::stop()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_isRunning)
            return;

        _isRunning = false;
        _requestProcessing = true;
    }
    _cv.notify_all();

    if (_thread.joinable())
    {
        _thread.join();
    }

    if (_onDestructionCallback)
    {
        _onDestructionCallback();
    }
}

void WorkerThread::wake()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _requestProcessing = true;
    }
    _cv.notify_one();
}

void WorkerThread::setOnStartAction(WTCallback onStartCallback) noexcept
{
    _onStartCallback = onStartCallback;
}

void WorkerThread::setOnDestructionAction(WTCallback onDestructionCallback) noexcept
{
    _onDestructionCallback = onDestructionCallback;
}

void WorkerThread::_process()
{
    while (_isRunning)
    {
        _isProcessing = true;
        process();
        _isProcessing = false;

        if (!_isRunning) break;

        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait_for(lock, std::chrono::milliseconds(_timeoutMs), [this]() {
            return !_isRunning || _requestProcessing;
        });

        _requestProcessing = false;
    }
}

}
