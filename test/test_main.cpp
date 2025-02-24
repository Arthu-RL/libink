#include <iostream>
#include "../include/vac/threadpool.h"
#include <chrono>

void runtime(const std::function<void()>& f) {
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
    vac::ThreadPool pool(4);

    // Submit tasks to the thread pool
    runtime([&](){
        auto result1 = pool.submit(add, 3, 4);
        auto result2 = pool.submit(add, 10, 20);
        auto result3 = pool.submit(add, 7, 8);
        auto result4 = pool.submit(add, 9, 8);
        // auto result5 = pool.submit(add, 10, 8);

        std::cout << "Result 1: " << result1.get() << std::endl;
        std::cout << "Result 2: " << result2.get() << std::endl;
        std::cout << "Result 3: " << result3.get() << std::endl;
        std::cout << "Result 4: " << result4.get() << std::endl;
        // std::cout << "Result 5: " << result5.get() << std::endl;
    });

    return 0;
}
