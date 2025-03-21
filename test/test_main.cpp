#include <iostream>
#include <chrono>

#include "../include/vac/vac.hpp"

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
    vac::ThreadPool pool(max_workers);
    std::vector<std::future<int>> futures;

    // Submit tasks to the thread pool
    runtime([&](){
        for (int i = 0; i < max_workers; ++i)
        {
            futures.push_back(pool.submit(add, max_workers, i));
        }

        for (int i = 0; i < max_workers; ++i)
        {
            std::cout << "Result " << i+1 << ": " << futures[i].get() << std::endl;
        }
        // pool.wait();
    });

    return 0;
}
