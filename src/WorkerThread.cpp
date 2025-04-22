#include "../include/ink/WorkerThread.h"
#include <chrono>

namespace ink {

WorkerThread::WorkerThread() :
    _isRunning(false),
    _isProcessing(false),
    _policy(Policy::KillImediately),
    _timeoutMs(0),
    _onStartCallback(nullptr),
    _onDestructionCallback(nullptr)
{
}

WorkerThread::WorkerThread(Policy policy) :
    _isRunning(false),
    _isProcessing(false),
    _policy(policy),
    _timeoutMs(0),
    _onStartCallback(nullptr),
    _onDestructionCallback(nullptr)
{
}

WorkerThread::WorkerThread(size_t timeoutSecs) :
    _isRunning(false),
    _isProcessing(false),
    _policy(Policy::WaitTimeout),
    _timeoutMs(timeoutSecs*1000),
    _onStartCallback(nullptr),
    _onDestructionCallback(nullptr)
{
}

WorkerThread::WorkerThread(Policy policy, size_t timeoutSecs) :
    _isRunning(false),
    _isProcessing(false),
    _policy(policy),
    _timeoutMs(timeoutSecs*1000),
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
    }

    // Release waiting
    _cv.notify_all();

    if (_policy == Policy::KillImediately)
    {
        if (_thread.joinable())
        {
            _thread.join();
        }
    }
    else if (_policy == Policy::WaitProcessFinish)
    {
        while (_isProcessing)
        {
            // Wait
        }

        if (_thread.joinable())
        {
            _thread.join();
        }
    }
    else if (_policy == Policy::WaitTimeout)
    {
        if (_thread.joinable())
        {
            _thread.join();
        }
    }

    if (_onDestructionCallback)
    {
        _onDestructionCallback();
    }
}

void WorkerThread::setOnStartAction(std::function<void()> onStartCallback)
{
    _onStartCallback = onStartCallback;
}

void WorkerThread::setOnDestructionAction(std::function<void()> onDestructionCallback)
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

        if (_policy == Policy::WaitTimeout && _timeoutMs > 0)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait_for(lock, std::chrono::milliseconds(_timeoutMs),
                         [this]() { return !_isRunning; });
        }
        else if (_timeoutMs > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(_timeoutMs));
        }
    }
}

}
