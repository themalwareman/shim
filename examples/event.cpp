#include <shim/event.h>

#include <iostream>
#include <thread>
#include <chrono>


int main() {
    // ------------------------------
    // 1. Auto-reset event example
    // ------------------------------
    shm::event auto_evt(shm::event::mode::auto_reset);

    std::thread t1([&] {
        std::cout << "[T1] Waiting for auto-reset event...\n";
        auto_evt.wait();
        std::cout << "[T1] Event fired!\n";
    });

    std::thread t2([&] {
        std::cout << "[T2] Waiting for auto-reset event...\n";
        auto_evt.wait();
        std::cout << "[T2] Event fired!\n";
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "[Main] Setting auto-reset event (should wake 1 thread)...\n";
    auto_evt.set(); // only one thread wakes

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "[Main] Setting auto-reset event again (wake the other thread)...\n";
    auto_evt.set();

    t1.join();
    t2.join();

    // ------------------------------
    // 2. Manual-reset event example
    // ------------------------------
    shm::event manual_evt(shm::event::mode::manual_reset);

    std::thread t3([&] {
        std::cout << "[T3] Waiting for manual-reset event...\n";
        manual_evt.wait();
        std::cout << "[T3] Event fired!\n";
    });

    std::thread t4([&] {
        std::cout << "[T4] Waiting for manual-reset event...\n";
        manual_evt.wait();
        std::cout << "[T4] Event fired!\n";
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "[Main] Setting manual-reset event (wakes all waiters)...\n";
    manual_evt.set(); // both threads wake

    t3.join();
    t4.join();

    // ------------------------------
    // 3. Try-wait example
    // ------------------------------
    shm::event evt(shm::event::mode::auto_reset);

    if (!evt.try_wait()) {
        std::cout << "[Main] Event not yet signaled\n";
    }

    evt.set();

    if (evt.try_wait()) {
        std::cout << "[Main] Event successfully consumed with try_wait()\n";
    }

    return 0;
}