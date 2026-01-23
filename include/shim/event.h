#pragma once

/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 themalwareman

    shim — small, header-only C++ utilities
    https://github.com/<your-username>/shim

    shm::event - a lightweight, thread-safe manual- or auto-reset event for signaling
                    between threads, providing set, reset, and wait semantics
                    without the boilerplate of condition variables.
*/

#include <condition_variable>
#include <mutex>
#include <chrono>

namespace shm {
    /*
        Notes:

        This is a lightweight, single-header C++ event abstraction, inspired by
        Windows-style events. Unlike std::condition_variable, which is stateless
        and requires separate mutex + boolean state, this class provides a simple
        reusable mechanism for threads to wait on a "signaled" condition.

        An event can operate in two modes:
            1. Manual-reset: stays signaled until explicitly reset.
            2. Auto-reset: automatically resets after releasing a single waiting thread.

        Typical usage patterns include:
            - Signaling worker threads to start or resume work or signal the main thread the worker is done.
            - Coordinating between threads without requiring complex condition_variable boilerplate.
            - Implementing one-shot or reusable thread notifications in a portable way.

        This has obviously been designed to be thread safe.

        Features:
            - Thread-safe: internally manages mutex and condition_variable.
            - Wait indefinitely (`wait()`), with timeout (`wait_for()`), or non-blocking (`try_wait()`).

        Example:
            shm::event evt(shm::event::mode::manual_reset);
            std::thread worker([&]{ / do work / evt.set(); });
            evt.wait();  // wait for worker thread to signal completion

    - Intended for general multi-threaded synchronization where you need a
        simple set/reset/wait pattern.
    - Does not store data: purely a signaling primitive.
    - Lightweight and exception-safe.

    */

    class event {
    public:
        /*
            We support 2 modes - manual and auto-reset. Use an enum because its
            more explicit than a bool.
        */
        enum class mode { manual_reset, auto_reset };

        /*
            // Constructors
            explicit event(mode mode = mode::manual_reset, bool signaled = false)

            // Copy construct and copy assign are deleted
            event(const event&) = delete;
            event& operator=(const event&) = delete;

            // Move construct and move assign are also deleted
            event(event&&) noexcept = delete;
            event& operator=(event&&) noexcept = delete;

            // Destructor
            ~event() = default;

            // Waiting
            void wait()
            template <typename Rep, typename Period>
            bool wait_for(const std::chrono::duration<Rep, Period>& timeout)
            template <typename Clock, typename Duration>
            bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point)
            bool try_wait()

            // Signalling
            void set()
            void reset()

        */

        // Construct event in non-signaled or initially signaled state
        explicit event(mode mode = mode::manual_reset, bool signaled = false)
            : _mode(mode), _signaled(signaled) {}

        // Non-copyable
        event(const event&) = delete;
        event& operator=(const event&) = delete;

        // Non-movable
        event(event&&) noexcept = delete;
        event& operator=(event&&) noexcept = delete;

        // Default destructor is sufficient
        ~event() = default;

        // Wait until event is signaled
        void wait() {
            // Lock the mutex
            std::unique_lock lock(_mutex);
            // Forward to cv wait, waiting on the signal bool
            _cv.wait(lock, [this]{ return _signaled; });
            // If we're auto_reset, reset the signal, cv returns with mutex locked
            // so we're safe to modify the signal
            if (mode::auto_reset == _mode) {
                _signaled = false;
            }
            // Mutex will be unlocked on unique_lock going out of scope
        }

        template <typename Rep, typename Period>
        bool wait_for(const std::chrono::duration<Rep, Period>& timeout)
        {
            // Grab mutex
            std::unique_lock<std::mutex> lock(_mutex);
            // Wait
            bool signaled = _cv.wait_for(lock, timeout, [this] { return _signaled; });
            // Auto-reset logic
            if (signaled && mode::auto_reset == _mode) {
                _signaled = false;
            }
            // Return if the event is signaled
            return signaled;
        }

        template <typename Clock, typename Duration>
        bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point)
        {
            // Grab mutex
            std::unique_lock<std::mutex> lock(_mutex);
            // Wait
            bool signaled = _cv.wait_until(lock, time_point, [this]{ return _signaled; });
            // Auto-reset logic
            if (signaled && mode::auto_reset == _mode) {
                _signaled = false;
            }
            // Return if the event is signaled
            return signaled;
        }

        // Check the event state without waiting, preserves auto-reset semantics
        bool try_wait() {
            std::lock_guard<std::mutex> lock(_mutex);
            bool signaled = _signaled;
            if (signaled && mode::auto_reset == _mode) {
                _signaled = false;
            }
            return signaled;
        }

        void set() {
            std::lock_guard<std::mutex> lock(_mutex);
            _signaled = true;

            if (mode::auto_reset == _mode) {
                _cv.notify_one();
            } else {
                _cv.notify_all();
            }
        }

        void reset() {
            std::lock_guard lock(_mutex);
            _signaled = false;
        }

    private:
        std::mutex _mutex;
        std::condition_variable _cv;
        bool _signaled;
        mode _mode;
    };

}


