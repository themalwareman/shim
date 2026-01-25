#include <iostream>
#include <vector>
#include <thread>

#include <shim/synchronized.h>

int main() {
    // Create a synchronized vector
    shm::synchronized<std::vector<int>> sync_vec;

    // -----------------------------
    // 1. Lambda-based exclusive access
    // -----------------------------
    sync_vec.with_exclusive([](auto& vec) {
        vec.push_back(10);
        vec.push_back(20);
    });

    // -----------------------------
    // 2. Lambda-based shared access
    // -----------------------------
    int sum = sync_vec.with_shared([](const auto& vec) {
        int s = 0;
        for (auto x : vec) s += x;
        return s;
    });
    std::cout << "Sum after first insertions: " << sum << "\n";

    // -----------------------------
    // 3. RAII-style exclusive lock
    // -----------------------------
    {
        auto guard = sync_vec.lock();   // exclusive access
        guard->push_back(30);
        guard->push_back(40);
        std::cout << "Size inside exclusive_guard: " << guard->size() << "\n";
    } // lock released automatically

    // -----------------------------
    // 4. RAII-style shared lock
    // -----------------------------
    {
        auto g_shared = sync_vec.lock_shared();
        std::cout << "Contents via shared_guard: ";
        for (auto val : *g_shared) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    } // shared lock released automatically

    // -----------------------------
    // 5. Multi-threaded example
    // -----------------------------
    auto thread_func = [&sync_vec](int start) {
        sync_vec.with_exclusive([start](auto& vec) {
            for (int i = start; i < start + 5; ++i) vec.push_back(i);
        });
    };

    std::thread t1(thread_func, 100);
    std::thread t2(thread_func, 200);

    t1.join();
    t2.join();

    // Read contents safely
    sync_vec.with_shared([](const auto& vec) {
        std::cout << "Vector after multi-threaded inserts: ";
        for (auto v : vec) std::cout << v << " ";
        std::cout << "\n";
    });

    return 0;
}