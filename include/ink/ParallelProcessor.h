#pragma once

#include <algorithm>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace ink
{

/**
 * Synchronous fork-join over a fixed set of workers.
 *
 * run() hands every worker a fixed stride of the index range and returns once
 * all of them are back, so there is nothing to allocate per dispatch: no task
 * node, no future, no shared state. That is the whole reason it exists next
 * to ThreadPool, whose submit() pays for all three.
 *
 * Concurrent callers serialize on one dispatch at a time. A body that calls
 * run() on the same processor executes that inner range inline on the
 * calling worker, so recursion cannot deadlock waiting for itself.
 */
class ParallelProcessor
{
  public:
    /**
     * @param concurrency Threads that take part in a dispatch, counting the
     *        caller: `concurrency - 1` workers are created. Clamped to 1.
     */
    explicit ParallelProcessor(std::size_t concurrency)
    {
        concurrency = std::max<std::size_t>(concurrency, 1);
        _workers.reserve(concurrency - 1);
        try
        {
            for (std::size_t i = 1; i < concurrency; ++i)
                _workers.emplace_back(
                    [this, i]
                    {
                        worker(i);
                    });
        }
        catch (...)
        {
            stop();
            throw;
        }
    }
    /** Joins the workers. No dispatch may be in progress on another thread. */
    ~ParallelProcessor()
    {
        stop();
    }
    ParallelProcessor(const ParallelProcessor &) = delete;
    ParallelProcessor &operator=(const ParallelProcessor &) = delete;

    /**
     * Calls `body(i)` for every `i` in `[0, count)` across the workers and the
     * caller, returning when all of them have finished.
     *
     * Every worker is joined before an exception leaves, so @p body may
     * capture locals by reference. The first exception thrown by any band is
     * rethrown; the rest are dropped. Runs inline when there are no workers,
     * when @p count is 1, or when called from inside its own body.
     *
     * @param body Invoked from several threads at once; must be safe for that.
     */
    void run(std::size_t count, const std::function<void(std::size_t)> &body)
    {
        if (!count)
            return;

        if (_active == this || _workers.empty() || count == 1)
        {
            for (std::size_t i = 0; i < count; ++i)
                body(i);
            return;
        }

        const std::lock_guard dispatch(_dispatchMutex);

        {
            const std::lock_guard lock(_mutex);
            _body = &body;
            _count = count;
            _remaining = _workers.size();
            _error = nullptr;
            ++_generation;
        }

        _ready.notify_all();
        invoke(0);
        std::unique_lock lock(_mutex);
        _done.wait(lock,
                   [this]
                   {
                       return _remaining == 0;
                   });
        _body = nullptr;
        if (_error)
            std::rethrow_exception(_error);
    }

  private:
    /** Runs @p band's stride of the current range, capturing its first exception. */
    void invoke(std::size_t band) noexcept
    {
        auto *previous = _active;
        _active = this;
        try
        {
            for (std::size_t i = band; i < _count; i += _workers.size() + 1)
                (*_body)(i);
        }
        catch (...)
        {
            const std::lock_guard lock(_mutex);
            if (!_error)
                _error = std::current_exception();
        }
        _active = previous;
    }

    /** Worker loop: waits for a new generation, runs its band, reports back. */
    void worker(std::size_t band)
    {
        std::size_t observed = 0;
        std::unique_lock lock(_mutex);
        while (true)
        {
            _ready.wait(lock,
                        [&]
                        {
                            return _stopping || _generation != observed;
                        });
            if (_stopping)
            {
                return;
            }
            observed = _generation;
            lock.unlock();
            invoke(band);
            lock.lock();
            if (--_remaining == 0)
            {
                _done.notify_one();
            }
        }
    }

    /** Wakes and joins every worker; safe to call more than once. */
    void stop() noexcept
    {
        {
            const std::lock_guard lock(_mutex);
            _stopping = true;
        }
        _ready.notify_all();
        for (auto &thread : _workers)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    /** The processor whose body this thread is currently inside, if any; what
     *  makes a recursive run() go inline instead of deadlocking. */
    inline static thread_local ParallelProcessor *_active = nullptr;

    /** Bands 1..N; band 0 is whichever thread called run(). */
    std::vector<std::thread> _workers;

    /** _dispatchMutex serializes callers of run(); _mutex guards the fields below. */
    std::mutex _dispatchMutex, _mutex;

    /** _ready wakes workers for a new generation; _done wakes the caller when the last band finishes. */
    std::condition_variable _ready, _done;

    /** The dispatch in progress. Points at the caller's argument, which outlives the dispatch. */
    const std::function<void(std::size_t)> *_body = nullptr;

    /** _count: range size; _remaining: workers still running; _generation: dispatch number workers compare against. */
    std::size_t _count = 0, _remaining = 0, _generation = 0;

    /** First exception raised by any band of the current dispatch. */
    std::exception_ptr _error;

    /** Set by stop(); workers exit their loop on seeing it. */
    bool _stopping = false;
};

} // namespace ink
