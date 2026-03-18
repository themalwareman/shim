
#include <catch2/catch_amalgamated.hpp>

#include <shim/event.h>

#include <vector>
#include <string>

/*
    test_event.cpp — Catch2 test suite for shm::event

    Covers:
        - Construction (default args, explicit mode, initial signal state)
        - Type traits (non-copyable, non-movable)
        - Manual-reset mode semantics
        - Auto-reset mode semantics
        - set() / reset() / try_wait()
        - wait_for() timeout and signal paths
        - wait_until() timeout and signal paths
        - Thread-safety and multi-waiter behaviour
        - Edge cases (set-before-wait, spurious-reset-safe, rapid set/reset)
*/



#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;


namespace {

// A short duration used as a "should not elapse" guard in signalling tests.
constexpr auto SIGNAL_TIMEOUT  = 500ms;

// A short duration used as the expected timeout in "nobody signals" tests.
// Keep small so tests stay fast; keep large enough to be reliable under load.
constexpr auto EXPECT_TIMEOUT  = 50ms;

// Spin up a thread that calls set() after a delay, then joins on scope exit.
struct DelayedSetter {
    explicit DelayedSetter(shm::event& ev, std::chrono::milliseconds delay = 20ms)
        : _thread([&ev, delay]{
            std::this_thread::sleep_for(delay);
            ev.set();
        }) {}

    ~DelayedSetter() { _thread.join(); }

private:
    std::thread _thread;
};

} // anonymous namespace


/*
    Event Tests
*/

TEST_CASE("constructors", "[event][construction]") {

    /*
        Test Cases:

        - Default is manual-reset non-signaled event
        - Explicit manual_reset, non-signaled
        - Explicit auto_reset, non-signaled
        - Initially signaled - manual_reset
        - Initially signaled - auto_reset
    */

    SECTION("Default is manual-reset non-signaled event") {
        shm::event ev;
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("Explicit manual_reset, non-signaled") {
        shm::event ev(shm::event::mode::manual_reset, false);
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("Explicit auto_reset, non-signaled") {
        shm::event ev(shm::event::mode::auto_reset, false);
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("Initially signaled - manual_reset") {
        shm::event ev(shm::event::mode::manual_reset, true);
        CHECK(ev.try_wait());
        CHECK(ev.try_wait());
    }

    SECTION("Initially signaled - auto_reset") {
        shm::event ev(shm::event::mode::auto_reset, true);
        CHECK(ev.try_wait());
        CHECK_FALSE(ev.try_wait());
    }
}

TEST_CASE("set and reset - manual_reset", "[event][set][reset]") {

    /*
        Test Cases:

        - Unsignaled event: try_wait returns false
        - After set: try_wait returns true repeatedly
        - After set then reset: try_wait returns false
        - Multiple set calls are idempotent
        - Multiple reset calls are idempotent
    */

    shm::event ev(shm::event::mode::manual_reset);

    SECTION("Unsignaled event: try_wait returns false") {
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("After set: try_wait returns true repeatedly") {
        ev.set();
        CHECK(ev.try_wait());
        CHECK(ev.try_wait());   // still signaled
        CHECK(ev.try_wait());
    }

    SECTION("After set then reset: try_wait returns false") {
        ev.set();
        ev.reset();
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("Multiple set calls are idempotent") {
        ev.set();
        ev.set();
        ev.set();
        CHECK(ev.try_wait());
        ev.reset();
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("Multiple reset calls are idempotent") {
        ev.reset();
        ev.reset();
        CHECK_FALSE(ev.try_wait());
    }
}

TEST_CASE("set and reset - auto_reset", "[event][set][reset]") {

    /*
        Test Cases:

        - Unsignaled event: try_wait returns false
        - After set: try_wait returns true exactly once
        - After set then reset: try_wait returns false
        - Set after consumed auto-reset re-arms the event
    */

    shm::event ev(shm::event::mode::auto_reset);

    SECTION("Unsignaled event: try_wait returns false") {
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("After set: try_wait returns true exactly once") {
        ev.set();
        CHECK(ev.try_wait());
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("After set then reset: try_wait returns false") {
        ev.set();
        ev.reset();
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("Set after consumed auto-reset re-arms the event") {
        ev.set();
        ev.try_wait();
        ev.set();
        CHECK(ev.try_wait());
        CHECK_FALSE(ev.try_wait());
    }
}

TEST_CASE("wait() - manual_reset", "[event][wait][manual_reset]") {

    /*
        Test Cases:

        - Thread signals waiter
        - Already signaled event returns immediately
    */

    SECTION("Thread signals waiter") {
        shm::event ev(shm::event::mode::manual_reset);
        std::atomic<bool> waiter_done{false};

        std::thread waiter([&]{
            ev.wait();
            waiter_done.store(true, std::memory_order_release);
        });

        std::this_thread::sleep_for(10ms);

        // Check its false
        CHECK_FALSE(waiter_done.load());

        // Unblock the waiter
        ev.set();
        // Wait for waiter to finish
        waiter.join();

        // Check its updated the bool
        CHECK(waiter_done.load());
        // Manual reset means we should be able to go straight through as its still signalled
        CHECK(ev.try_wait());
    }

    SECTION("Already signaled event returns immediately") {
        shm::event ev(shm::event::mode::manual_reset, true);

        auto start = std::chrono::steady_clock::now();
        ev.wait();
        auto elapsed = std::chrono::steady_clock::now() - start;

        CHECK(elapsed < 20ms);
        CHECK(ev.try_wait());
    }

}

TEST_CASE("wait() — auto_reset", "[event][wait][auto_reset]") {

    /*
        Test Cases:

        - Thread signals waiter
    */

    SECTION("Thread signals waiter") {
        shm::event ev(shm::event::mode::auto_reset);
        std::atomic<bool> waiter_done{false};

        std::thread waiter([&]{
            ev.wait();
            waiter_done.store(true, std::memory_order_release);
        });

        std::this_thread::sleep_for(10ms);
        ev.set();
        waiter.join();

        CHECK(waiter_done.load());
        // Check event is no longer signaled
        CHECK_FALSE(ev.try_wait());
    }
}

TEST_CASE("wait_for()", "[event][wait_for]") {

    /*
        Test Cases:

        - Returns false when not signaled
        - Returns true if signaled
        - Returns immediately if already signaled
        - Auto-reset rests after successful wait_for
        - Manual reset stays signaled after successful wait_for
        - Zero timeout acts as non-blocking check
    */

    SECTION("Returns false when not signaled") {
        shm::event ev(shm::event::mode::manual_reset);

        auto start   = std::chrono::steady_clock::now();
        bool result  = ev.wait_for(EXPECT_TIMEOUT);
        auto elapsed = std::chrono::steady_clock::now() - start;

        CHECK_FALSE(result);
        CHECK(elapsed >= EXPECT_TIMEOUT);
    }

    SECTION("Returns true if signaled") {
        shm::event ev(shm::event::mode::manual_reset);
        DelayedSetter setter(ev, 20ms);

        bool result = ev.wait_for(SIGNAL_TIMEOUT);

        CHECK(result);
    }

    SECTION("Returns immediately if already signaled") {
        shm::event ev(shm::event::mode::manual_reset, /*signaled=*/true);

        auto start   = std::chrono::steady_clock::now();
        bool result  = ev.wait_for(SIGNAL_TIMEOUT);
        auto elapsed = std::chrono::steady_clock::now() - start;

        CHECK(result);
        CHECK(elapsed < 20ms);
    }

    SECTION("Auto-reset rests after successful wait_for") {
        shm::event ev(shm::event::mode::auto_reset, /*signaled=*/true);

        CHECK(ev.wait_for(SIGNAL_TIMEOUT));
        CHECK_FALSE(ev.try_wait());
    }

    SECTION("Manual reset stays signaled after successful wait_for") {
        shm::event ev(shm::event::mode::manual_reset, /*signaled=*/true);

        CHECK(ev.wait_for(SIGNAL_TIMEOUT));
        CHECK(ev.try_wait());
    }

    SECTION("Zero timeout acts as non-blocking check") {
        shm::event ev(shm::event::mode::manual_reset);

        auto start   = std::chrono::steady_clock::now();
        CHECK_FALSE(ev.wait_for(0ms));
        auto elapsed = std::chrono::steady_clock::now() - start;
        CHECK(elapsed < 20ms);

        ev.set();
        start   = std::chrono::steady_clock::now();
        CHECK(ev.wait_for(0ms));
        elapsed = std::chrono::steady_clock::now() - start;
        CHECK(elapsed < 20ms);
    }
}

TEST_CASE("wait_until()", "[event][wait_until]") {

    /*
        Test Cases:

        - Returns false when deadline already passed
        - Returns false on timeout when not signaled
        - Returns true when signaled before deadline
        - Auto_reset resets after successful wait_until
    */

    SECTION("Returns false when deadline already passed") {
        shm::event ev(shm::event::mode::manual_reset);
        auto past = std::chrono::steady_clock::now() - 1ms;

        CHECK_FALSE(ev.wait_until(past));
    }

    SECTION("Returns false on timeout when not signaled") {
        shm::event ev(shm::event::mode::manual_reset);
        auto deadline = std::chrono::steady_clock::now() + EXPECT_TIMEOUT;

        auto start   = std::chrono::steady_clock::now();
        bool result  = ev.wait_until(deadline);
        auto elapsed = std::chrono::steady_clock::now() - start;

        CHECK_FALSE(result);
        CHECK(elapsed >= EXPECT_TIMEOUT);
    }

    SECTION("Returns true when signaled before deadline") {
        shm::event ev(shm::event::mode::manual_reset);
        DelayedSetter setter(ev, 20ms);

        auto deadline = std::chrono::steady_clock::now() + SIGNAL_TIMEOUT;
        bool result   = ev.wait_until(deadline);

        CHECK(result);
    }

    SECTION("Auto_reset resets after successful wait_until") {
        shm::event ev(shm::event::mode::auto_reset, /*signaled=*/true);
        auto deadline = std::chrono::steady_clock::now() + SIGNAL_TIMEOUT;

        CHECK(ev.wait_until(deadline));
        CHECK_FALSE(ev.try_wait());
    }
}

TEST_CASE("Multiple waiters", "[event][multi_waiter]") {
    /*
        Test Cases:

        - Manual-reset wakes all waiters on set()
        - Auto-reset wakes exactly one waiter per set()
    */

    SECTION("Manual-reset wakes all waiters on set()") {
        constexpr int NUM_WAITERS = 8;
        shm::event ev(shm::event::mode::manual_reset);
        std::atomic<int> woken{0};

        std::vector<std::thread> threads;
        threads.reserve(NUM_WAITERS);

        for (int i = 0; i < NUM_WAITERS; ++i) {
            threads.emplace_back([&]{
                ev.wait();
                woken.fetch_add(1, std::memory_order_relaxed);
            });
        }

        std::this_thread::sleep_for(20ms);   // let all threads block
        ev.set();

        for (auto& t : threads) t.join();

        CHECK(woken.load() == NUM_WAITERS);
        CHECK(ev.try_wait());
    }

    SECTION("Auto-reset wakes exactly one waiter per set()") {
        constexpr int NUM_SIGNALS = 5;
        shm::event ev(shm::event::mode::auto_reset);
        std::atomic<int> woken{0};

        // NUM_SIGNALS waiters all pile up
        std::vector<std::thread> threads;
        threads.reserve(NUM_SIGNALS);

        for (int i = 0; i < NUM_SIGNALS; ++i) {
            threads.emplace_back([&]{
                ev.wait();
                woken.fetch_add(1, std::memory_order_relaxed);
            });
        }

        std::this_thread::sleep_for(20ms);   // let all threads block

        // Fire one signal per waiter
        for (int i = 0; i < NUM_SIGNALS; ++i) {
            ev.set();
            // Give the released thread time to increment woken before next set
            std::this_thread::sleep_for(5ms);
            CHECK(woken.load() == i + 1);
        }

        for (auto& t : threads) t.join();

        CHECK(woken.load() == NUM_SIGNALS);
        CHECK_FALSE(ev.try_wait());
    }
}




// ===========================================================================
// Section 8 — Thread-safety stress test
// ===========================================================================

TEST_CASE("Concurrent set/reset/try_wait does not corrupt state", "[event][thread_safety]") {

    shm::event ev(shm::event::mode::manual_reset);
    std::atomic<bool> stop{false};
    constexpr int NUM_THREADS = 4;
    constexpr auto DURATION   = 200ms;

    // Hammers set/reset concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, i]{
            while (!stop.load(std::memory_order_relaxed)) {
                if (i % 2 == 0) {
                    ev.set();
                    ev.try_wait();
                } else {
                    ev.reset();
                    ev.try_wait();
                }
            }
        });
    }

    std::this_thread::sleep_for(DURATION);
    stop.store(true);
    for (auto& t : threads) t.join();

    // After stopping we can still set/check cleanly — no UB / crash = pass
    ev.set();
    CHECK(ev.try_wait());
    ev.reset();
    CHECK_FALSE(ev.try_wait());
}

TEST_CASE("Concurrent wait_for stress — no deadlock or crash", "[event][thread_safety]") {

    shm::event ev(shm::event::mode::auto_reset);
    constexpr int ROUNDS     = 20;
    constexpr int NUM_WAITERS = 4;

    for (int r = 0; r < ROUNDS; ++r) {
        std::atomic<int> woken{0};
        std::vector<std::thread> threads;

        for (int i = 0; i < NUM_WAITERS; ++i) {
            threads.emplace_back([&]{
                if (ev.wait_for(100ms)) {
                    woken.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        // Signal them one-by-one
        for (int i = 0; i < NUM_WAITERS; ++i) {
            std::this_thread::sleep_for(5ms);
            ev.set();
        }

        for (auto& t : threads) t.join();
        CHECK(woken.load() == NUM_WAITERS);
    }
}


// ===========================================================================
// Section 9 — Edge cases
// ===========================================================================

TEST_CASE("Set before wait — wait returns without blocking", "[event][edge_cases]") {

    shm::event ev(shm::event::mode::manual_reset);
    ev.set();

    auto start   = std::chrono::steady_clock::now();
    ev.wait();
    auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(elapsed < 20ms);
}

TEST_CASE("Rapid set/reset cycle does not lose signal under single thread", "[event][edge_cases]") {

    shm::event ev(shm::event::mode::manual_reset);
    for (int i = 0; i < 1000; ++i) {
        ev.set();
        CHECK(ev.try_wait());
        ev.reset();
        CHECK_FALSE(ev.try_wait());
    }
}

TEST_CASE("Auto-reset: rapid set/try_wait cycle", "[event][edge_cases][auto_reset]") {

    shm::event ev(shm::event::mode::auto_reset);
    for (int i = 0; i < 1000; ++i) {
        ev.set();
        CHECK(ev.try_wait());
        CHECK_FALSE(ev.try_wait());   // already consumed
    }
}

TEST_CASE("Reset on unsignaled manual-reset event is a no-op", "[event][edge_cases]") {

    shm::event ev(shm::event::mode::manual_reset);
    ev.reset();
    CHECK_FALSE(ev.try_wait());
    // Now set and verify it works normally after a spurious reset
    ev.set();
    CHECK(ev.try_wait());
}

TEST_CASE("wait_for() with very large timeout is interruptible by set()", "[event][edge_cases]") {

    shm::event ev(shm::event::mode::manual_reset);
    std::atomic<bool> done{false};

    std::thread t([&]{
        // This would block for a very long time without the signal
        bool result = ev.wait_for(60s);
        done.store(result, std::memory_order_release);
    });

    std::this_thread::sleep_for(20ms);
    ev.set();
    t.join();

    CHECK(done.load());
}

TEST_CASE("Manual-reset event can be reused after reset", "[event][edge_cases]") {

    shm::event ev(shm::event::mode::manual_reset);
    std::atomic<int> counter{0};

    constexpr int CYCLES = 3;
    for (int c = 0; c < CYCLES; ++c) {
        std::thread worker([&]{
            ev.wait();
            counter.fetch_add(1, std::memory_order_relaxed);
        });

        std::this_thread::sleep_for(10ms);
        ev.set();
        worker.join();
        ev.reset();
    }

    CHECK(counter.load() == CYCLES);
}