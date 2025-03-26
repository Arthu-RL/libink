#include <iostream>
#include <chrono>

#include "../include/ink/ink.hpp"

void runtime(std::function<void()>&& f) {
    auto start = std::chrono::high_resolution_clock::now();

    f();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << duration.count() << " ms" << '\n';
}

int add(int a, int b) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return a + b;
}

int main() {
    // Create a thread pool with 4 threads
    int max_workers = 4;
    ink::ThreadPool pool(max_workers);
    // std::vector<std::future<int>> futures;

    ink::LogManager::getInstance().setGlobalLevel(ink::LogLevel::TRACE);

    INK_DEBUG << "test";
    INK_TRACE << "test";
    INK_VERBOSE << "test";
    INK_INFO << "test";
    INK_WARN << "test";
    INK_ERROR << "test";
    INK_FATAL << "test";

    INK_ASSERT(1==1);
    INK_ASSERT_MSG(1==1, "TEST");

    // Submit tasks to the thread pool
    runtime([&](){
        for (int i = 0; i < max_workers; ++i)
        {
            pool.submit(add, max_workers, i);
        }

        pool.wait();
    });

    INK_DEBUG << "async";

    return 0;
}
