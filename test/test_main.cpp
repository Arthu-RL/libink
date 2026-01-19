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
    for (;;)
    {

    }
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    return a + b;
}

class TestWorkerThread : public ink::WorkerThread
{
public:
    TestWorkerThread(ink::WorkerThread::Policy policy = ink::WorkerThread::Policy::WaitTimeout, size_t timeoutSecs = 0) :
        WorkerThread(policy, timeoutSecs),
        _processCount(0)
    {
        setOnStartAction([this]() { INK_LOG << "TestWorkerThread started"; });
        setOnDestructionAction([this]() { INK_LOG << "TestWorkerThread destroyed"; });
    }

    size_t getProcessCount() const { return _processCount; }

    void resetProcessCount() { _processCount = 0; }

protected:
    virtual void process() override
    {
        _processCount++;

        INK_LOG << "Process called: " << _processCount << " times";
    }

private:
    std::atomic<size_t> _processCount;
};


int main(int argc, char** argv) {
    ink::ArgParser argParser("INK argParser TEST");
    argParser.add_argument("-w", "--workers", "workers", "Threads para trabalhar no processamento em paralelo", "22", false);
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

    TestWorkerThread worker(ink::WorkerThread::Policy::WaitProcessFinish, 1);

    worker.start();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    worker.stop();

    INK_ASSERT(1==1);
    INK_ASSERT_MSG(1==1, "TEST");

    INK_LOG << ink::utils::exec_command("nvidia-smi");

    // Submit tasks to the thread pool
    runtime([&](){
        for (int i = 0; i < max_workers; ++i)
        {
            futures.push_back(pool.submit(add, max_workers, i));
        }

        for (auto& future : futures)
        {
            future.get();
        }
    });

    runtime([&](){
        // Create a new list
        ink::InkedList<int>list;

        // Test empty list properties
        INK_DEBUG << "Empty list test:";
        INK_DEBUG << "  Length: " << list.length();
        INK_DEBUG << "  Head null? " << (list.head() == nullptr ? "Yes" : "No");

        // Test push_back
        INK_DEBUG << "\nAdding elements with push_back:";
        int val1 = 10, val2 = 20, val3 = 30;

        list.push_back(val1);
        list.push_back(val2);
        list.push_back(val3);

        INK_DEBUG << "  Length after adding 3 elements: " << list.length();

        // Test forward traversal
        INK_DEBUG << "  List contents:";
        auto* curr = list.head();
        while (curr != nullptr) {
            INK_DEBUG << "    Value: " << curr->data;
            curr = curr->next;
        }

        // Test insert
        INK_DEBUG << "\nInserting element at position 1:";
        int val4 = 15;
        list.insert(val4, 1);

        INK_DEBUG << "  Length after insert: " << list.length();
        INK_DEBUG << "  List contents:";
        curr = list.head();
        while (curr != nullptr) {
            INK_DEBUG << "    Value: " << curr->data;
            curr = curr->next;
        }

        // Test remove by index
        INK_DEBUG << "\nRemoving element at index 2:";
        if (list.remove_idx(2))
        {
            INK_DEBUG << "  Length after remove: " << list.length();
            INK_DEBUG << "  List contents:";
            curr = list.head();
            while (curr != nullptr) {
                INK_DEBUG << "    Value: " << curr->data;
                curr = curr->next;
            }
        }
        else
        {
            INK_FATAL << "FAil remove index";
        }

        if (list.remove_data(val3))
        {
            // Test remove by value
            INK_DEBUG << "\nRemoving element with value 30:";

            INK_DEBUG << "  Length after remove: " << list.length();
            INK_DEBUG << "  List contents:";
            curr = list.head();
            while (curr != nullptr) {
                INK_DEBUG << "    Value: " << curr->data;
                curr = curr->next;
            }
        }
        else
        {
            INK_FATAL << "Fail remove element";
        }

        // Test pop_front with data capture
        INK_DEBUG << "\nPopping from front:";
        int* popped_data = nullptr;
        list.pop_front(popped_data);

        INK_DEBUG << "  Popped value: " << popped_data;
        INK_DEBUG << "  Length after pop_front: " << list.length();
        INK_DEBUG << "  List contents:";
        curr = list.head();
        while (curr != nullptr) {
            INK_DEBUG << "    Value: " << curr->data;
            curr = curr->next;
        }

        // Test enqueue (add to front)
        INK_DEBUG << "\nEnqueuing new element at front:";
        int val5 = 5;
        list.enqueue(val5);

        INK_DEBUG << "  Length after enqueue: " << list.length();
        INK_DEBUG << "  List contents:";
        curr = list.head();
        while (curr != nullptr) {
            INK_DEBUG << "    Value: " << curr->data;
            curr = curr->next;
        }

        // Test pop_back
        INK_DEBUG << "\nPopping from back:";
        list.pop_back(popped_data);

        INK_DEBUG << "  Popped value: " << popped_data;
        INK_DEBUG << "  Length after pop_back: " << list.length();
        INK_DEBUG << "  List contents:";
        curr = list.head();
        while (curr != nullptr) {
            INK_DEBUG << "    Value: " << curr->data;
            curr = curr->next;
        }

        // Test edge case: operations on empty list
        INK_DEBUG << "\nTesting operations on empty list:";

        // Clear the list first
        while (list.length() > 0) {
            list.pop_front(nullptr);
        }

        INK_DEBUG << "  Length of empty list: " << list.length();

        // Try popping from empty list
        list.pop_front(popped_data);
        list.pop_back(popped_data);

        INK_DEBUG << "  Length after popping from empty list: " << list.length();

        // Add element to empty list
        INK_DEBUG << "\nAdding to previously emptied list:";
        int val6 = 42;
        list.push_back(val6);

        INK_DEBUG << "  Length after adding to empty list: " << list.length();
        INK_DEBUG << "  List contents:";
        curr = list.head();
        while (curr != nullptr) {
            INK_DEBUG << "    Value: " << curr->data;
            curr = curr->next;
        }

        ////////////////////////////////
        /// TESTING CRYPT
        ////////////////////////////////

        std::string to_encrypt = "Testing OTP crypt...";
        std::string key = ink::crypt::OTP::build_key(to_encrypt.size(), 123, 10000);

        const std::string secret_file = "/tmp/secret.txt";
        if (!ink::crypt::OTP::write_to_file(secret_file, key))
        {
            std::cout << "Failed to write to secret file" << '\n';
            throw std::runtime_error("Tests failed at writing to file");
        }

        std::string secret = ink::crypt::OTP::read_from_file(secret_file);
        std::string encrypted_msg = ink::crypt::OTP::encrypt(to_encrypt, secret);
        std::string decrypted_msg = ink::crypt::OTP::decrypt(encrypted_msg, secret);

        if (to_encrypt != decrypted_msg)
        {
            throw std::runtime_error("Tests failed at encryted/decrypted comparisson");
        }

        std::cout << "message: " << to_encrypt << "\nkey: " << key << "\nEncrypted message: " << encrypted_msg << "\nDecrypted message: " << decrypted_msg << '\n';

        return 0;
    });

    return 0;
}
