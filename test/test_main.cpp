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

int main(int argc, char** argv) {
    ink::ArgParser argParser("INK argParser TEST");
    argParser.add_argument("-w", "--workers", "workers", "Threads para trabalhar no processamento em paralelo", "10", false);
    argParser.add_argument("-t", "--test", "test", "test", "test", false);
    ink::EnhancedJson args = argParser.parse_args(ink::ArgParser::argsToString(argc, argv));
    argParser.show_help();

    int max_workers = std::stoi(args.get<std::string>("workers"));
    ink::ThreadPool pool(max_workers);
    std::vector<std::future<int>> futures;

    INK_LOG << "max_workers " << max_workers;
    INK_LOG << ink::ArgParser::argsToString(argc, argv);

    ink::LogManager::getInstance().setGlobalLevel(ink::LogLevel::TRACE);
    ink::LogManager::getInstance().setLogToFile("./test.log");

    INK_DEBUG << "test " << args.get<std::string>("test");
    INK_TRACE << "test";
    INK_VERBOSE << "test";
    INK_INFO << "test";
    INK_WARN << "test";
    INK_ERROR << "test";
    INK_FATAL << "test";

    INK_ASSERT(1==1);
    INK_ASSERT_MSG(1==1, "TEST");

    // Submit tasks to the thread pool
    // runtime([&](){
    //     for (int i = 0; i < max_workers; ++i)
    //     {
    //         futures.push_back(pool.submit(add, max_workers, i));
    //     }

    //     for (auto& future : futures)
    //     {
    //         future.get();
    //     }
    // });

    // runtime([&](){
    //     // Create a new list
    //     ink::InkedList<int>list;

    //     // Test empty list properties
    //     INK_DEBUG << "Empty list test:";
    //     INK_DEBUG << "  Length: " << list.length();
    //     INK_DEBUG << "  Head null? " << (list.head() == nullptr ? "Yes" : "No");

    //     // Test push_back
    //     INK_DEBUG << "\nAdding elements with push_back:";
    //     int val1 = 10, val2 = 20, val3 = 30;

    //     list.push_back(val1);
    //     list.push_back(val2);
    //     list.push_back(val3);

    //     INK_DEBUG << "  Length after adding 3 elements: " << list.length();

    //     // Test forward traversal
    //     INK_DEBUG << "  List contents:";
    //     auto* curr = list.head();
    //     while (curr != nullptr) {
    //         INK_DEBUG << "    Value: " << curr->data;
    //         curr = curr->next;
    //     }

    //     // Test insert
    //     INK_DEBUG << "\nInserting element at position 1:";
    //     int val4 = 15;
    //     list.insert(val4, 1);

    //     INK_DEBUG << "  Length after insert: " << list.length();
    //     INK_DEBUG << "  List contents:";
    //     curr = list.head();
    //     while (curr != nullptr) {
    //         INK_DEBUG << "    Value: " << curr->data;
    //         curr = curr->next;
    //     }

    //     // Test remove by index
    //     INK_DEBUG << "\nRemoving element at index 2:";
    //     if (list.remove(2))
    //     {
    //         INK_DEBUG << "  Length after remove: " << list.length();
    //         INK_DEBUG << "  List contents:";
    //         curr = list.head();
    //         while (curr != nullptr) {
    //             INK_DEBUG << "    Value: " << curr->data;
    //             curr = curr->next;
    //         }
    //     }
    //     else
    //     {
    //         INK_FATAL << "FAil remove index";
    //     }

    //     if (list.remove(val3))
    //     {
    //         // Test remove by value
    //         INK_DEBUG << "\nRemoving element with value 30:";

    //         INK_DEBUG << "  Length after remove: " << list.length();
    //         INK_DEBUG << "  List contents:";
    //         curr = list.head();
    //         while (curr != nullptr) {
    //             INK_DEBUG << "    Value: " << curr->data;
    //             curr = curr->next;
    //         }
    //     }
    //     else
    //     {
    //         INK_FATAL << "Fail remove element";
    //     }

    //     // Test pop_front with data capture
    //     INK_DEBUG << "\nPopping from front:";
    //     int* popped_data = nullptr;
    //     list.pop_front(popped_data);

    //     INK_DEBUG << "  Popped value: " << popped_data;
    //     INK_DEBUG << "  Length after pop_front: " << list.length();
    //     INK_DEBUG << "  List contents:";
    //     curr = list.head();
    //     while (curr != nullptr) {
    //         INK_DEBUG << "    Value: " << curr->data;
    //         curr = curr->next;
    //     }

    //     // Test enqueue (add to front)
    //     INK_DEBUG << "\nEnqueuing new element at front:";
    //     int val5 = 5;
    //     list.enqueue(val5);

    //     INK_DEBUG << "  Length after enqueue: " << list.length();
    //     INK_DEBUG << "  List contents:";
    //     curr = list.head();
    //     while (curr != nullptr) {
    //         INK_DEBUG << "    Value: " << curr->data;
    //         curr = curr->next;
    //     }

    //     // Test pop_back
    //     INK_DEBUG << "\nPopping from back:";
    //     list.pop_back(popped_data);

    //     INK_DEBUG << "  Popped value: " << popped_data;
    //     INK_DEBUG << "  Length after pop_back: " << list.length();
    //     INK_DEBUG << "  List contents:";
    //     curr = list.head();
    //     while (curr != nullptr) {
    //         INK_DEBUG << "    Value: " << curr->data;
    //         curr = curr->next;
    //     }

    //     // Test edge case: operations on empty list
    //     INK_DEBUG << "\nTesting operations on empty list:";

    //     // Clear the list first
    //     while (list.length() > 0) {
    //         list.pop_front(nullptr);
    //     }

    //     INK_DEBUG << "  Length of empty list: " << list.length();

    //     // Try popping from empty list
    //     list.pop_front(popped_data);
    //     list.pop_back(popped_data);

    //     INK_DEBUG << "  Length after popping from empty list: " << list.length();

    //     // Add element to empty list
    //     INK_DEBUG << "\nAdding to previously emptied list:";
    //     int val6 = 42;
    //     list.push_back(val6);

    //     INK_DEBUG << "  Length after adding to empty list: " << list.length();
    //     INK_DEBUG << "  List contents:";
    //     curr = list.head();
    //     while (curr != nullptr) {
    //         INK_DEBUG << "    Value: " << curr->data;
    //         curr = curr->next;
    //     }
    // });

    INK_DEBUG << "async";

    return 0;
}
