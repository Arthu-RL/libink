#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <variant>

#include "../include/ink/ink.hpp"

// ============================================================================
// Minimal assertion-based test harness (no external test framework dependency)
// ============================================================================
namespace test {

inline int g_total = 0;
inline int g_failed = 0;

inline void check(bool cond, const char* expr, const char* file, int line) {
    ++g_total;
    if (!cond) {
        ++g_failed;
        INK_ERROR << "CHECK FAILED: " << expr << " (" << file << ":" << line << ")";
    }
}

} // namespace test

#define CHECK(cond) ::test::check((cond), #cond, __FILE__, __LINE__)
#define SECTION(name) INK_LOG << "\n========== " name " =========="

void runtime(std::function<void()>&& f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    INK_LOG << "Runtime duration: " << duration.count() << " ms";
}

// ============================================================================
// utils
// ============================================================================
void test_utils()
{
    SECTION("utils");

    CHECK(ink::utils::cto_int('0') == 0);
    CHECK(ink::utils::cto_int('7') == 7);
    CHECK(ink::utils::cto_int('9') == 9);
    CHECK(ink::utils::cto_int('a') == -1);
    CHECK(ink::utils::cto_int('!') == -1);

    auto parsed = ink::utils::string_int("12345");
    CHECK(parsed.has_value());
    CHECK(parsed.value_or(0) == 12345);

    auto badParsed = ink::utils::string_int("not_a_number");
    CHECK(!badParsed.has_value());

    u64 t1 = ink::utils::nowMillis();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    u64 t2 = ink::utils::nowMillis();
    CHECK(t2 >= t1);

    auto execResult = ink::utils::exec_command("echo ink_exec_test");
    CHECK(execResult.has_value());
    if (execResult.has_value()) {
        CHECK(execResult->find("ink_exec_test") != std::string::npos);
    }
}

// ============================================================================
// Inkogger / LogManager
// ============================================================================
void test_inkogger()
{
    SECTION("Inkogger/LogManager");

    auto logger1 = ink::LogManager::getInstance().getLogger("TEST_LOGGER");
    auto logger2 = ink::LogManager::getInstance().getLogger("TEST_LOGGER");
    CHECK(logger1 == logger2); // Same name must return the same instance

    auto core1 = ink::LogManager::getInstance().getCoreLogger();
    auto core2 = ink::LogManager::getInstance().getCoreLogger();
    CHECK(core1 == core2);

    logger1->setLevel(ink::LogLevel::WARN);
    CHECK(logger1->isEnabled(ink::LogLevel::FATAL));
    CHECK(logger1->isEnabled(ink::LogLevel::ERROR));
    CHECK(logger1->isEnabled(ink::LogLevel::WARN));
    CHECK(!logger1->isEnabled(ink::LogLevel::INFO));
    CHECK(!logger1->isEnabled(ink::LogLevel::TRACE));

    logger1->setLevel(ink::LogLevel::TRACE);
    CHECK(logger1->isEnabled(ink::LogLevel::TRACE));

    logger1->setName("RENAMED_LOGGER");
    CHECK(logger1->getName() == "RENAMED_LOGGER");

    // File logging: write a line, then verify the file actually contains it.
    const std::string logPath = "./ink_logger_test.log";
    logger1->setLogToFile(logPath);
    logger1->log(ink::LogLevel::INFO, "file logging smoke test", __FILE__, __LINE__);
    logger1->setLogToFile(""); // close/flush

    std::ifstream in(logPath);
    CHECK(in.is_open());
    if (in.is_open()) {
        std::stringstream ss;
        ss << in.rdbuf();
        CHECK(ss.str().find("file logging smoke test") != std::string::npos);
    }

    // Regression test: concurrent logging to the same file must not corrupt
    // or drop lines (see the lock-free O_APPEND file-write path).
    const std::string concurrentLogPath = "./ink_logger_concurrent_test.log";
    std::remove(concurrentLogPath.c_str());

    auto stressLogger = ink::LogManager::getInstance().getLogger("CONCURRENT_STRESS");
    stressLogger->setLevel(ink::LogLevel::TRACE);
    stressLogger->setUseColors(false);
    stressLogger->setLogToFile(concurrentLogPath);

    constexpr int kThreads = 4;
    constexpr int kLinesPerThread = 250;
    {
        std::vector<std::thread> threads;
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([stressLogger, t]() {
                for (int i = 0; i < kLinesPerThread; ++i) {
                    INKL_INFO(stressLogger) << "thread=" << t << " line=" << i;
                }
            });
        }
        for (auto& th : threads) th.join();
    }
    stressLogger->setLogToFile("");

    std::ifstream concurrentIn(concurrentLogPath);
    int lineCount = 0;
    bool corrupted = false;
    std::string line;
    while (std::getline(concurrentIn, line)) {
        if (line.find("line=") == std::string::npos) corrupted = true;
        ++lineCount;
    }
    CHECK(!corrupted);
    CHECK(lineCount == kThreads * kLinesPerThread);

    std::remove(logPath.c_str());
    std::remove(concurrentLogPath.c_str());
}

// ============================================================================
// InkAssert
// ============================================================================
void test_inkassert()
{
    SECTION("InkAssert");

    // Only the passing path is exercised here: a failing INK_ASSERT traps
    // the process by design, so it cannot be checked in-process.
    INK_ASSERT(1 == 1);
    INK_ASSERT_MSG(2 + 2 == 4, "basic arithmetic must hold");
    CHECK(true);
}

// ============================================================================
// RingBuffer
// ============================================================================
void test_ringbuffer()
{
    SECTION("RingBuffer");

    ink::RingBuffer rb(16);
    CHECK(rb.capacity() == 16);
    CHECK(rb.empty());
    CHECK(!rb.full());
    CHECK(rb.size() == 0);

    size_t written = rb.write("hello", 5);
    CHECK(written == 5);
    CHECK(rb.size() == 5);
    CHECK(!rb.empty());

    char readBuf[16] = {};
    size_t readCount = rb.read(readBuf, 5);
    CHECK(readCount == 5);
    CHECK(std::string(readBuf, 5) == "hello");
    CHECK(rb.empty());

    // Wrap-around: fill, drain partially, refill to force the write/read
    // pointers to cross the physical end of the buffer.
    rb.clear();
    CHECK(rb.write("0123456789", 10) == 10);
    char drain[8] = {};
    CHECK(rb.read(drain, 6) == 6); // readPos now at 6
    CHECK(rb.write("ABCDEF", 6) == 6); // writePos wraps around past capacity=16

    char full[16] = {};
    size_t remaining = rb.read(full, 16);
    CHECK(remaining == 10);
    CHECK(std::string(full, 10) == "6789ABCDEF");

    // Full buffer: writes beyond capacity are truncated, not overflowed.
    rb.clear();
    CHECK(rb.write(std::string(20, 'x')) == 16);
    CHECK(rb.full());
    CHECK(rb.write("y", 1) == 0); // no space left

    // Zero-copy accessors
    rb.clear();
    rb.write("zerocpy", 7);
    size_t avail = 0;
    const char* readPtr = rb.getReadBuffer(avail);
    CHECK(readPtr != nullptr);
    CHECK(avail == 7);
    rb.advanceReadPos(7);
    CHECK(rb.empty());

    size_t space = 0;
    char* writePtr = rb.getWriteBuffer(space);
    CHECK(writePtr != nullptr);
    CHECK(space > 0);

    // Move semantics (RingBuffer was previously non-movable due to a raw
    // new[]/delete[] buffer; now backed by std::vector<char>).
    ink::RingBuffer moveSrc(32);
    moveSrc.write("movable", 7);
    ink::RingBuffer moveDst(std::move(moveSrc));
    CHECK(moveDst.size() == 7);
    char moveOut[8] = {};
    moveDst.read(moveOut, 7);
    CHECK(std::string(moveOut, 7) == "movable");
}

// ============================================================================
// ObjectPool
// ============================================================================
struct PoolProbe {
    static inline int liveCount = 0;
    static inline int constructCount = 0;
    static inline int destructCount = 0;

    int value;

    explicit PoolProbe(int v) : value(v) {
        ++liveCount;
        ++constructCount;
    }
    ~PoolProbe() {
        --liveCount;
        ++destructCount;
    }
};

void test_objectpool()
{
    SECTION("ObjectPool");

    PoolProbe::liveCount = 0;
    PoolProbe::constructCount = 0;
    PoolProbe::destructCount = 0;

    {
        ink::ObjectPool<PoolProbe, 4> pool;

        // acquire() must actually placement-construct T (previously it
        // handed back raw, uninitialized storage).
        PoolProbe* a = pool.acquire(42);
        CHECK(a->value == 42);
        CHECK(PoolProbe::liveCount == 1);
        CHECK(PoolProbe::constructCount == 1);

        PoolProbe* b = pool.acquire(7);
        CHECK(b->value == 7);
        CHECK(PoolProbe::liveCount == 2);

        // release() must call the destructor (previously it didn't).
        pool.release(a);
        CHECK(PoolProbe::liveCount == 1);
        CHECK(PoolProbe::destructCount == 1);

        // Reacquire should reuse freed storage and construct a fresh object.
        PoolProbe* c = pool.acquire(99);
        CHECK(c->value == 99);
        CHECK(PoolProbe::liveCount == 2);

        pool.release(b);
        pool.release(c);
        CHECK(PoolProbe::liveCount == 0);

        // Force expansion beyond the initial slab (iSize=4).
        std::vector<PoolProbe*> many;
        for (int i = 0; i < 20; ++i) {
            many.push_back(pool.acquire(i));
        }
        CHECK(PoolProbe::liveCount == 20);
        for (PoolProbe* p : many) {
            pool.release(p);
        }
        CHECK(PoolProbe::liveCount == 0);

        CHECK(pool.getRawBuffer() != nullptr);
        CHECK(pool.getRawBufferSize() > 0);
    }
}

// ============================================================================
// ArenaAllocator (InkedArena)
// ============================================================================
void test_arena_allocator()
{
    SECTION("ArenaAllocator");

    ink::InkedArena arena;
    ink::InkedArena::Arena a{};
    arena.arena_init(&a, 4096);

    void* p1 = arena.arena_alloc(&a, 64, alignof(std::max_align_t));
    CHECK(p1 != nullptr);
    CHECK((reinterpret_cast<std::uintptr_t>(p1) % alignof(std::max_align_t)) == 0);

    void* p2 = arena.arena_alloc(&a, 128, 16);
    CHECK(p2 != nullptr);
    CHECK((reinterpret_cast<std::uintptr_t>(p2) % 16) == 0);
    CHECK(p2 != p1);

    // Regression test: a large, exactly-block-sized allocation with a wide
    // alignment requirement used to spuriously fail because the freshly
    // created block had no slack reserved for alignment padding.
    void* big = arena.arena_alloc(&a, 8192, 64);
    CHECK(big != nullptr);
    CHECK((reinterpret_cast<std::uintptr_t>(big) % 64) == 0);

    arena.arena_reset(&a);
    void* afterReset = arena.arena_alloc(&a, 64, 8);
    CHECK(afterReset != nullptr);

    arena.arena_destroy(&a);
    CHECK(a.head == nullptr);
}

// ============================================================================
// AlignedAllocator
// ============================================================================
void test_aligned_allocator()
{
    SECTION("AlignedAllocator");

    ink::AlignedAllocator<f32, 64> alloc;
    f32* mem = alloc.allocate(16);
    CHECK(mem != nullptr);
    CHECK((reinterpret_cast<std::uintptr_t>(mem) % 64) == 0);
    for (int i = 0; i < 16; ++i) mem[i] = static_cast<f32>(i);
    CHECK(mem[15] == 15.0f);
    alloc.deallocate(mem, 16);

    // Must be usable as a standard allocator with std containers.
    std::vector<f32, ink::AlignedAllocator<f32, 32>> alignedVec;
    alignedVec.resize(8, 1.0f);
    CHECK((reinterpret_cast<std::uintptr_t>(alignedVec.data()) % 32) == 0);
    CHECK(alignedVec.size() == 8);
}

// ============================================================================
// ThreadPool
// ============================================================================
int add(int a, int b) { return a + b; }

void test_threadpool()
{
    SECTION("ThreadPool");

    bool threwOnZeroWorkers = false;
    try {
        ink::ThreadPool zeroPool(0);
    } catch (const std::invalid_argument&) {
        threwOnZeroWorkers = true;
    }
    CHECK(threwOnZeroWorkers); // regression: 0 workers used to hang forever

    constexpr int kWorkers = 4;
    ink::ThreadPool pool(kWorkers);
    std::vector<std::future<int>> futures;

    runtime([&]() {
        for (int i = 0; i < 50; ++i) {
            futures.push_back(pool.submit(add, i, i * 2));
        }

        int expectedSum = 0, actualSum = 0;
        for (int i = 0; i < 50; ++i) {
            expectedSum += i + i * 2;
            actualSum += futures[i].get();
        }
        CHECK(expectedSum == actualSum);
    });
}

// ============================================================================
// WorkerThread
// ============================================================================
class TestWorkerThread : public ink::WorkerThread
{
public:
    TestWorkerThread(ink::WorkerThread::Policy policy, size_t timeoutSecs) :
        WorkerThread(policy, timeoutSecs),
        _processCount(0)
    {
        setOnStartAction([this]() { INK_LOG << "TestWorkerThread started"; });
        setOnDestructionAction([this]() { INK_LOG << "TestWorkerThread destroyed"; });
    }

    size_t getProcessCount() const { return _processCount.load(); }

protected:
    void process() override
    {
        _processCount++;
    }

private:
    std::atomic<size_t> _processCount;
};

void test_workerthread()
{
    SECTION("WorkerThread");

    // WaitProcessFinish: stop() must block until the worker thread fully exits.
    {
        TestWorkerThread worker(ink::WorkerThread::Policy::WaitProcessFinish, 1);
        CHECK(!worker.isRunning());
        worker.start();
        CHECK(worker.isRunning());
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        worker.wake();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        worker.stop();
        CHECK(!worker.isRunning());
        CHECK(worker.getProcessCount() >= 1);
    }

    // WaitTimeout: stop() detaches instead of blocking (see WorkerThread.cpp).
    {
        TestWorkerThread worker(ink::WorkerThread::Policy::WaitTimeout, 1);
        worker.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        worker.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CHECK(!worker.isRunning());
    }
}

// ============================================================================
// Queue
// ============================================================================
void test_queue()
{
    SECTION("Queue");

    ink::Queue<int> q;
    CHECK(q.empty());

    q.push(1);
    q.push(2);
    q.push(3);
    CHECK(q.size() == 3);

    int value = 0;
    CHECK(q.try_pop(value));
    CHECK(value == 1);
    CHECK(q.size() == 2);

    CHECK(q.wait_and_pop(value));
    CHECK(value == 2);

    auto popped = q.pop_front();
    CHECK(popped.has_value());
    CHECK(popped.value() == 3);
    CHECK(q.empty());

    CHECK(!q.try_pop(value));

    std::vector<int> bulk{10, 20, 30, 40};
    q.push_bulk(bulk.begin(), bulk.end());
    CHECK(q.size() == 4);

    int sum = 0;
    while (q.try_pop(value)) sum += value;
    CHECK(sum == 100);

    bool poppedAfterTimeout = q.try_pop_for(value, std::chrono::milliseconds(10));
    CHECK(!poppedAfterTimeout); // empty queue, should time out

    // wait_and_pop must unblock (returning false) once shutdown() is called.
    std::thread waiter([&]() {
        int v = 0;
        bool got = q.wait_and_pop(v);
        CHECK(!got);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    q.shutdown();
    waiter.join();
    CHECK(q.is_shutdown());
}

// ============================================================================
// TimerWheel
// ============================================================================
void test_timerwheel()
{
    SECTION("TimerWheel");

    constexpr u32 kTicksToLive = 4;
    ink::TimerWheel wheel(kTicksToLive, 10); // 4 ticks to live, 10ms per tick

    std::array<ink::TimerNode, 3> nodes{};
    for (auto& n : nodes) wheel.update(&n);

    // unlink() removes a node before it expires.
    wheel.unlink(&nodes[1]);

    int expiredCount = 0;
    bool sawUnlinkedNode = false;

    // A node inserted while the hand is at slot 0 lands in bucket
    // `ticksToLive` (newSlot = current + ticksToLive). tick() returns the
    // bucket at the hand's *current* position before advancing it, so
    // reaching that bucket takes ticksToLive + 1 calls.
    for (u32 i = 0; i < kTicksToLive + 1; ++i) {
        wheel.processExpired([&](ink::TimerNode* node) {
            ++expiredCount;
            if (node == &nodes[1]) sawUnlinkedNode = true;
        });
    }

    CHECK(expiredCount == 2); // nodes[0] and nodes[2], not the unlinked one
    CHECK(!sawUnlinkedNode);

    u64 next = wheel.timeToNextTickMillis(wheel.getNextTickTime());
    CHECK(next == 0); // already at/after the next tick boundary
}

// ============================================================================
// InkedList
// ============================================================================
struct MoveProbe {
    int value;
    static inline int copyCount = 0;
    static inline int moveCount = 0;

    MoveProbe(int v) : value(v) {}
    MoveProbe(const MoveProbe& other) : value(other.value) { ++copyCount; }
    MoveProbe(MoveProbe&& other) noexcept : value(other.value) { ++moveCount; other.value = -1; }
    MoveProbe& operator=(const MoveProbe&) = default;
    MoveProbe& operator=(MoveProbe&&) = default;
    bool operator==(const MoveProbe& other) const { return value == other.value; }
};

void test_inkedlist()
{
    SECTION("InkedList");

    ink::InkedList<int> list;
    CHECK(list.length() == 0);
    CHECK(list.head() == nullptr);

    int val1 = 10, val2 = 20, val3 = 30;
    list.push_back(val1);
    list.push_back(val2);
    list.push_back(val3);
    CHECK(list.length() == 3);

    int val4 = 15;
    list.insert(val4, 1);
    CHECK(list.length() == 4);

    {
        auto* curr = list.head();
        std::vector<int> observed;
        while (curr) { observed.push_back(curr->data); curr = curr->next; }
        CHECK((observed == std::vector<int>{10, 15, 20, 30}));
    }

    CHECK(list.remove_idx(2));
    CHECK(list.length() == 3);

    CHECK(list.remove_data(val3));
    CHECK(list.length() == 2);

    int popped = 0;
    CHECK(list.pop_front(&popped));
    CHECK(popped == 10);
    CHECK(list.length() == 1);

    int val5 = 5;
    list.enqueue(val5);
    CHECK(list.length() == 2);
    CHECK(list.head()->data == 5);

    CHECK(list.pop_back(&popped));
    CHECK(list.length() == 1);

    while (list.length() > 0) list.pop_front(nullptr);
    CHECK(list.length() == 0);
    CHECK(!list.pop_front(nullptr));
    CHECK(!list.pop_back(nullptr));

    int val6 = 42;
    list.push_back(val6);
    CHECK(list.length() == 1);
    CHECK(list.head()->data == 42);

    // Regression test: insert(T&&, index) must actually move, not silently
    // copy the named rvalue-reference parameter.
    {
        ink::InkedList<MoveProbe> moveList;
        moveList.push_back(MoveProbe(1));
        moveList.push_back(MoveProbe(2));

        MoveProbe::copyCount = 0;
        MoveProbe::moveCount = 0;
        moveList.insert(MoveProbe(99), 1); // interior insert path
        CHECK(MoveProbe::copyCount == 0);
        CHECK(MoveProbe::moveCount >= 1);

        MoveProbe::copyCount = 0;
        MoveProbe::moveCount = 0;
        moveList.insert(MoveProbe(100), 0); // index==0 -> enqueue path
        CHECK(MoveProbe::copyCount == 0);

        MoveProbe::copyCount = 0;
        MoveProbe::moveCount = 0;
        moveList.insert(MoveProbe(101), 999); // out-of-range -> push_back path
        CHECK(MoveProbe::copyCount == 0);
    }

    // Two-argument (header, data) constructors. Note: head() returns the
    // `data` node, not `header_data` -- the ctor links header -> data and
    // head() always returns `root`, which this ctor points at `data`.
    {
        ink::InkedList<int> headed(1, 2);
        CHECK(headed.length() == 2);
        CHECK(headed.head()->data == 2);
        CHECK(headed.head()->prev != nullptr);
        CHECK(headed.head()->prev->data == 1);
    }
}

// ============================================================================
// InkixTree
// ============================================================================
void test_inkixtree()
{
    SECTION("InkixTree");

    ink::InkixTree<int> tree;
    tree.insert("test", 1);
    tree.insert("team", 2);
    tree.insert("toast", 3);
    tree.insert("apple", 4);

    CHECK(tree.get("test") != nullptr);
    CHECK(*tree.get("test") == 1);
    CHECK(*tree.get("team") == 2);
    CHECK(*tree.get("toast") == 3);
    CHECK(*tree.get("apple") == 4);

    // Regression test: querying a nonexistent key must not dereference a
    // null pointer -- get()/getCopy() previously did exactly that.
    CHECK(tree.get("nonexistent") == nullptr);
    CHECK(tree.get("te") == nullptr); // a prefix of "test"/"team", never inserted
    CHECK(!tree.getCopy("nonexistent").has_value());
    CHECK(tree.getCopy("apple").value_or(-1) == 4);

    // Regression test: a key that only happens to land exactly on an
    // internal branch-split point (never itself inserted) must not be
    // reported as found.
    ink::InkixTree<int> prefixTree;
    prefixTree.insert("apple", 100);
    prefixTree.insert("application", 200);
    CHECK(prefixTree.get("appl") == nullptr); // branch point, not an inserted key
    CHECK(*prefixTree.get("apple") == 100);
    CHECK(*prefixTree.get("application") == 200);

    // Regression test: re-inserting an existing key must update its value,
    // not just leave the old value in place.
    tree.insert("test", 999);
    CHECK(*tree.get("test") == 999);
}

// ============================================================================
// InkOtp
// ============================================================================
void test_inkotp()
{
    SECTION("InkOtp (OTP XOR cipher)");

    std::string plaintext = "Testing OTP crypt...";
    std::string key = ink::crypt::OTP::build_key(plaintext.size(), 123, 10000);
    CHECK(key.size() == plaintext.size());

    std::string encrypted = ink::crypt::OTP::encrypt(plaintext, key);
    std::string decrypted = ink::crypt::OTP::decrypt(encrypted, key);
    CHECK(decrypted == plaintext);
    CHECK(encrypted != plaintext);

    // Regression test: two keys generated back-to-back must differ (the RNG
    // used to be seeded from second-precision wall-clock time alone, so
    // rapid consecutive calls produced identical "random" keys).
    std::string key2 = ink::crypt::OTP::build_key(plaintext.size(), 123, 10000);
    CHECK(key != key2);

    // Regression test: mismatched key/text length must not silently read
    // out of bounds -- it must fail loudly instead.
    bool threwOnShortKey = false;
    try {
        ink::crypt::OTP::encrypt(plaintext, "short");
    } catch (const std::invalid_argument&) {
        threwOnShortKey = true;
    }
    CHECK(threwOnShortKey);

    // File round-trip.
    const std::string secretFile = "./ink_otp_test_secret.bin";
    CHECK(ink::crypt::OTP::write_to_file(secretFile, key));
    std::string readBack = ink::crypt::OTP::read_from_file(secretFile);
    CHECK(readBack == key);
    std::remove(secretFile.c_str());

    CHECK(ink::crypt::OTP::read_from_file("./this_file_should_not_exist.bin").empty());
}

// ============================================================================
// InkType
// ============================================================================
void test_inktype()
{
    SECTION("InkType");

    ink::InkType invalid;
    CHECK(!invalid.isValid());
    CHECK(invalid.getType() == ink::InkType::InkTypeId::Invalid);

    ink::InkType i(static_cast<i32>(-42));
    CHECK(i.isValid());
    CHECK(i.getType() == ink::InkType::InkTypeId::I32);
    CHECK(std::get<i32>(i.toVariant()) == -42);

    ink::InkType u(static_cast<u64>(123456789));
    CHECK(std::get<u64>(u.toVariant()) == 123456789);

    ink::InkType f(3.14f);
    CHECK(f.getType() == ink::InkType::InkTypeId::F32);

    ink::InkType b(true);
    CHECK(std::get<bool>(b.toVariant()) == true);

    ink::InkType str(std::string("hello ink"));
    CHECK(str.getType() == ink::InkType::InkTypeId::String);
    CHECK(std::get<std::string>(str.toVariant()) == "hello ink");

    // Copy must deep-copy the string, not alias the same allocation.
    ink::InkType strCopy(str);
    ink::InkType strReassigned(std::string("mutated"));
    strCopy = strReassigned;
    CHECK(std::get<std::string>(str.toVariant()) == "hello ink"); // original untouched
    CHECK(std::get<std::string>(strCopy.toVariant()) == "mutated");

    // Move must leave the source Invalid (and not double-free on destruction).
    ink::InkType strMoveSrc(std::string("move me"));
    ink::InkType strMoveDst(std::move(strMoveSrc));
    CHECK(strMoveDst.getType() == ink::InkType::InkTypeId::String);
    CHECK(std::get<std::string>(strMoveDst.toVariant()) == "move me");
    CHECK(!strMoveSrc.isValid());
}

// ============================================================================
// LastWish
// ============================================================================
void test_lastwish()
{
    SECTION("LastWish");

    bool started = false;
    bool finished = false;

    {
        ink::LastWish wish(
            [&]() { started = true; CHECK(!finished); },
            [&]() { finished = true; }
        );
        CHECK(started);
        CHECK(!finished);
    }

    CHECK(finished);
}

// ============================================================================
// ArgParser
// ============================================================================
void test_argparser()
{
    SECTION("ArgParser");

    ink::ArgParser parser("Test parser");
    parser.add_argument("-n", "--name", "name", "a name value", "default_name", false);
    parser.add_argument("--required-flag", "required", "a required flag", "", true);

    ink::EnhancedJson args = parser.parse_args("--name=alice --required-flag=present");
    CHECK(args.get<std::string>("name") == "alice");
    CHECK(args.get<std::string>("required") == "present");

    // Missing optional argument falls back to its default.
    ink::EnhancedJson argsDefaulted = parser.parse_args("--required-flag=present");
    CHECK(argsDefaulted.get<std::string>("name") == "default_name");

    // Missing required argument must throw.
    bool threwOnMissingRequired = false;
    try {
        parser.parse_args("--name=bob");
    } catch (const std::runtime_error&) {
        threwOnMissingRequired = true;
    }
    CHECK(threwOnMissingRequired);

    CHECK(ink::ArgParser::argsToString(0, nullptr).empty());
}

// ============================================================================
// EnhancedJson / EnhancedJsonUtils
// ============================================================================
void test_enhancedjson()
{
    SECTION("EnhancedJson/EnhancedJsonUtils");

    ink::EnhancedJson obj = ink::EnhancedJson::object();
    obj["name"] = "ink";
    obj["version"] = 1;
    obj["nested"]["flag"] = true;

    CHECK(obj.get<std::string>("name") == "ink");
    CHECK(obj.get<int>("version") == 1);
    CHECK(obj.get<std::string>("missing_key", "fallback") == "fallback");

    CHECK(obj.getPath<bool>("/nested/flag", false) == true);
    obj.setPath("/nested/other", 42);
    CHECK(obj.getPath<int>("/nested/other", -1) == 42);

    std::string dumped = obj.dump();
    CHECK(!dumped.empty());

    ink::EnhancedJson arr = ink::EnhancedJson::array();
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    CHECK(arr.get<int>(size_t(0), -1) == 1);
    CHECK(arr.get<int>(size_t(5), -1) == -1); // out of bounds -> default

    // EnhancedJsonUtils
    CHECK(ink::EnhancedJsonUtils::isObject(obj));
    CHECK(!ink::EnhancedJsonUtils::isArray(obj));
    CHECK(ink::EnhancedJsonUtils::isArray(arr));
    CHECK(ink::EnhancedJsonUtils::size(arr) == 3);
    CHECK(ink::EnhancedJsonUtils::hasKey(obj, "name"));
    CHECK(!ink::EnhancedJsonUtils::hasKey(obj, "nope"));

    auto keys = ink::EnhancedJsonUtils::getKeys(obj);
    CHECK(std::find(keys.begin(), keys.end(), "name") != keys.end());

    ink::EnhancedJson parsed = ink::EnhancedJsonUtils::loadFromString(R"({"a":1,"b":2})");
    CHECK(parsed.get<int>("a") == 1);

    ink::EnhancedJson base = ink::EnhancedJsonUtils::loadFromString(R"({"a":1,"b":2})");
    ink::EnhancedJson overlay = ink::EnhancedJsonUtils::loadFromString(R"({"b":3,"c":4})");
    ink::EnhancedJson merged = ink::EnhancedJsonUtils::merge(base, overlay);
    CHECK(merged.get<int>("a") == 1);
    CHECK(merged.get<int>("b") == 3);
    CHECK(merged.get<int>("c") == 4);

    CHECK(ink::EnhancedJsonUtils::getTypeName(arr) == "array");
    CHECK(ink::EnhancedJsonUtils::getTypeName(obj) == "object");

    // Binary round-trip (CBOR).
    std::vector<u8> binary = ink::EnhancedJsonUtils::toBinary(obj);
    CHECK(!binary.empty());
    ink::EnhancedJson fromBinary = ink::EnhancedJsonUtils::fromBinary(binary);
    CHECK(fromBinary.get<std::string>("name") == "ink");

    // File round-trip.
    const std::string jsonFile = "./ink_json_test.json";
    CHECK(ink::EnhancedJsonUtils::saveToFile(obj, jsonFile));
    ink::EnhancedJson loaded = ink::EnhancedJsonUtils::loadFromFile(jsonFile);
    CHECK(loaded.get<std::string>("name") == "ink");
    std::remove(jsonFile.c_str());
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char** argv)
{
    ink::LogManager::getInstance().setGlobalLevel(ink::LogLevel::TRACE);

    INK_TRACE << "test";
    INK_VERBOSE << "test";
    INK_INFO << "test";
    INK_WARN << "test";
    INK_ERROR << "test";
    INK_FATAL << "test";
    INK_LOG << "raw log line, argc=" << argc;
    INK_UNUSED(argv);

    test_utils();
    test_inkogger();
    test_inkassert();
    test_ringbuffer();
    test_objectpool();
    test_arena_allocator();
    test_aligned_allocator();
    test_threadpool();
    test_workerthread();
    test_queue();
    test_timerwheel();
    test_inkedlist();
    test_inkixtree();
    test_inkotp();
    test_inktype();
    test_lastwish();
    test_argparser();
    test_enhancedjson();

    INK_LOG << "\n==================================================";
    INK_LOG << "TESTS: " << (test::g_total - test::g_failed) << "/" << test::g_total << " passed";
    INK_LOG << "==================================================";

    return test::g_failed == 0 ? 0 : 1;
}
