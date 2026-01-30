#pragma once

/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 themalwareman

    shim — small, header-only C++ utilities
    https://github.com/themalwareman/shim

    shm::synchronized<T> - a small utility for safe, scoped access to shared data,
                    combining a mutex and an object into a single abstraction.
*/

#include <type_traits>
#include <concepts>
#include <utility>
#include <shared_mutex>
#include <mutex>
#include <functional>

namespace shm {

    /*
        Notes:

        We require that the template type T is:

        Not a reference - Feels like this would be a mistake from the user, making access to
            a reference synchronous doesn't really guarantee any safety over the original
            object the reference is pointing to.

        Not const - If the object is const and only readable then there's nothing to synchronize
            because all access would be reads so there's no reason to synchronize the access as nothing
            is changing.

        Not void - Obviously a mistake, no point synchronizing access to nothing.


    */

    template<typename T>
    requires (not std::is_reference_v<T>) &&
        (not std::is_const_v<T>) &&
        (not std::is_void_v<T>)
    class synchronized {

        public:

        /*
            // Constructors - forwarding allows specific instantiation of guarded data
            synchronized() requires std::default_initializable<T> = default;
            template <typename... Args>
            requires std::constructible_from<T, Args...>
            explicit synchronized(Args&&... args)

            // Copy construct and copy assign are deleted
            synchronized(const synchronized&) = delete;
            synchronized& operator=(const synchronized&) = delete;

            // Move construct and move assign are also deleted
            synchronized(synchronized&&) = delete;
            synchronized& operator=(synchronized&&) = delete;

            // Destructor
            ~synchronized() = default;

            // Invocable Access
            template <typename F>
            requires std::invocable<F, T&>
            auto with_exclusive(F&& f)

            template <typename F>
            requires std::invocable<F, const T&>
            auto with_shared(F&& f) const

            // Legacy Access
            [[nodiscard]] exclusive_guard lock()
            [[nodiscard]] shared_guard lock_shared()


            // Signalling
            void set()
            void reset()

        */

        synchronized() requires std::default_initializable<T> = default;

        // Forwarding constructor to allow args to be passed in to construct a T
        template <typename... Args>
        requires std::constructible_from<T, Args...>
        explicit synchronized(Args&&... args) : _value(std::forward<Args>(args)...) {}

        // Non-copyable
        synchronized(const synchronized&) = delete;
        synchronized& operator=(const synchronized&) = delete;

        // Non-movable
        synchronized(synchronized&&) = delete;
        synchronized& operator=(synchronized&&) = delete;

        // Destructor - default is enough
        ~synchronized() = default;

        /*
            Invocable Access

            Note: with_* returns by value to prevent references to protected data
            from escaping the synchronized region. Use lock()/lock_shared() for
            reference-based access.

            Also, callers need to ensure their lambda takes a reference if they wish
            to update the state. There is no clean way I know of to avoid anyone supplying
            a lambda that would take a copy of the data.
        */
        template <typename F>
        requires std::invocable<F, T&>
        auto with_exclusive(F&& f) {
            std::unique_lock lock(_mutex);
            return std::invoke(std::forward<F>(f), _value);
        }

        template <typename F>
        requires std::invocable<F, const T&>
        auto with_shared(F&& f) const {
            std::shared_lock lock(_mutex);
            return std::invoke(std::forward<F>(f), _value);
        }

        /*
            Legacy Access
        */

        class exclusive_guard {
            friend class synchronized<T>;
        private:
            exclusive_guard(std::unique_lock<std::shared_mutex> lock, T* ptr)
                : _lock(std::move(lock)), _ptr(ptr) {}
        public:
            // Disallow copy of guard
            exclusive_guard(const exclusive_guard&) = delete;
            exclusive_guard& operator=(const exclusive_guard&) = delete;

            // Allow move to people can return x.lock() etc
            exclusive_guard(exclusive_guard&&) = default;
            exclusive_guard& operator=(exclusive_guard&&) = default;

            // Provide -> operator to allow access to contained type
            [[nodiscard]] T* operator->() noexcept { return _ptr; }
            [[nodiscard]] T& operator*() noexcept { return *_ptr; }

        private:
            std::unique_lock<std::shared_mutex> _lock;
            T* _ptr;
        };

        class shared_guard {
            friend class synchronized<T>;

        private:
            shared_guard(std::shared_lock<std::shared_mutex> lock, const T* ptr)
                : _lock(std::move(lock)), _ptr(ptr) {}
        public:
            // No copy
            shared_guard(const shared_guard&) = delete;
            shared_guard& operator=(const shared_guard&) = delete;

            // Allow move
            shared_guard(shared_guard&&) = default;
            shared_guard& operator=(shared_guard&&) = default;

            // Access
            [[nodiscard]] const T* operator->() const noexcept { return _ptr; }
            [[nodiscard]] const T& operator*() const noexcept { return *_ptr; }

        private:
            std::shared_lock<std::shared_mutex> _lock;
            const T* _ptr;
        };

        // With the RAII guards defined let's provide the legacy API
        [[nodiscard]] exclusive_guard lock() {
            return exclusive_guard(std::unique_lock(_mutex), &_value);
        }

        [[nodiscard]] shared_guard lock_shared() const {
            return shared_guard(std::shared_lock(_mutex), &_value);
        }


    private:
        T _value;
        mutable std::shared_mutex _mutex;
    };
}