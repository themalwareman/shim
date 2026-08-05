#pragma once

/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 themalwareman

    shim - small, header-only C++ utilities
    https://github.com/themalwareman/shim

    shm::task<T> - a cooperative task with deferred exception
            propagation built on jthread with stop token support,
            enables deferred launch and provides the ability for
            tasks to return values.
*/

#include "event.h"

#include <concepts>
#include <thread>
#include <optional>
#include <type_traits>
#include <functional>
#include <list>
#include <stop_token>
#include <atomic>
#include <exception>
#include <utility>
#include <variant>
#include <chrono>
#include <cstdint>

/*
 * shm::task<T> is non-movable because our callable wrapper takes a this pointer
 *  to be able to set the completion event. Allowing moves would cause issues here.
 *  For anyone looking to run multiple tasks the container is left up to the user.
 *  For running tasks of the same type i.e. task<void> say a sensible option would
 *  be a std::list as it doesn't require the type to be movable. This then allows
 *  you to do group operations by simply iterating the std::list.
 */

namespace shm {

    // Template on return type of task
    template<typename R>
    requires std::move_constructible<R> || std::same_as<R, void>
    class task {

    public:

        enum class launch_policy { immediate, deferred };

        using tid = uint32_t;

        [[nodiscard]] tid id() const noexcept { return _id; }

        /*
            // Constructors
            template<typename F>
            requires std::constructible_from<std::function<R(std::stop_token)>, F>
            explicit task(F&& callable, launch_policy policy = launch_policy::immediate) : _callable(std::forward<F>(callable))

            template<typename F, typename S>
            requires std::constructible_from<std::function<R(std::stop_token)>, F> &&
                std::constructible_from<std::function<void()>, S>
            explicit task(F&& callable, S&& stop_callback, launch_policy policy = launch_policy::immediate)

            // Copy construct and copy assign are deleted
            task(const task&) = delete;
            task& operator=(const task&) = delete;

            // Move construct and move assign are also deleted
            task(task&&) = delete;
            task& operator=(task&&) = delete;

            // Destructor
            ~task()

            // Task
            void start()
            void request_stop()
            void request_stop_and_wait()
            [[nodiscard]] bool started() const noexcept

            // Waiting
            void wait() const
            template <typename Rep, typename Period>
            [[nodiscard]] bool wait_for(const std::chrono::duration<Rep, Period>& timeout) const
            template <typename Clock, typename Duration>
            [[nodiscard]] bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point) const
            [[nodiscard]] bool try_wait() const

            // Result collection
            R get()

        */

        /*
         * I want to force any users to have to take a stop_token so that willfully
         * ignoring it is at least a choice rather than likely a mistake. We also
         * for now require a std::function to be constructible from the callable
         * because for now we use a std::function to store it.
         */
        template<typename F>
        requires std::constructible_from<std::function<R(std::stop_token)>, F>
        explicit task(F&& callable, launch_policy policy = launch_policy::immediate) : _callable(std::forward<F>(callable))
        {
            // Are we launching the work right away?
            if (policy == launch_policy::immediate) {
                this->start();
            }
        }

        /*
         * We also mandate that stop callbacks are parameterless and void return, it simplifies storage
         * as it stops the template diverging further
         */
        template<typename F, typename S>
        requires std::constructible_from<std::function<R(std::stop_token)>, F> &&
            std::constructible_from<std::function<void()>, S>
        explicit task(F&& callable, S&& stop_callback, launch_policy policy = launch_policy::immediate)
            : _callable(std::forward<F>(callable)), _stop_callback(std::forward<S>(stop_callback))
        {
            // Are we launching the work right away?
            if (policy == launch_policy::immediate) {
                this->start();
            }
        }

        /*
            Copy/Assign/Move
        */

        // Non-copyable
        task(const task&) = delete;
        task& operator=(const task&) = delete;

        // Non-movable
        task(task&&) = delete;
        task& operator=(task&&) = delete;

        /*
            Destructor
        */
        ~task()
        {
            /*
                jthread takes a this pointer to set the completion event, to prevent member destruction
                ordering issues we wait on the join in our own destructor first to guarantee the thread
                has exited.
            */
            if (_thread.has_value()) {
                _thread->request_stop();
                _thread->join();
            }
        }

        /*
            Task
        */

        void start() {
            if (_thread.has_value()) {
                throw std::runtime_error("task already started");
            }

            _thread = std::jthread([this](std::stop_token token) -> void {
                run_task(token);
            });

            if (_stop_callback) {
                _stop_callback_wrapper.emplace(_thread->get_stop_token(), _stop_callback);
            }
        }

        void request_stop() {
            if (not _thread.has_value()) {
                throw std::runtime_error("task not started");
            }

            _thread->request_stop();
        }

        void request_stop_and_wait() {
            request_stop();
            _completion_event.wait();
        }

        [[nodiscard]] bool started() const noexcept {
            return _thread.has_value();
        }

        /*
            Waiting
        */

        void wait() const {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            _completion_event.wait();
        }

        template <typename Rep, typename Period>
        [[nodiscard]] bool wait_for(const std::chrono::duration<Rep, Period>& timeout) const {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            return _completion_event.wait_for(timeout);
        }

        template <typename Clock, typename Duration>
        [[nodiscard]] bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point) const {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            return _completion_event.wait_until(time_point);
        }

        [[nodiscard]] bool try_wait() const {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            return _completion_event.try_wait();
        }

        /*
            Result Collection
        */
        R get() {
            // Wait for completion
            wait();

            // Check if an exception was thrown
            if (_exception) {
                std::rethrow_exception(_exception);
            }

            // No exception so return result if non-void
            if constexpr(not std::is_void_v<R>) {
                if (not _result) {
                    throw std::runtime_error("result already collected from task");
                }
                R retval = std::move(_result.value());
                _result.reset();
                return retval;
            }
            // Or just return if void - helps IDE see all paths return
            else {
                return;
            }
        }

    private:

        // Centralised code path
        void run_task(const std::stop_token& token) {

            /*
             * We could grab the stop token from the jthread here, but it's a bit janky
             * so we'll just get the jthread creator to pass it through to us.
             */

            try {
                // Invoke the callable passing the stop_token
                if constexpr (std::is_void_v<R>) {
                    _callable(token);
                } else {
                    _result.emplace(_callable(token));
                }
            }
            catch(...) {
                // Save off the exception to propagate later
                _exception = std::current_exception();
            }

            // Work is done
            _completion_event.set();
        }

    private:
        // Callable to invoke upon task start
        std::function<R(std::stop_token)> _callable;
        // Optional stop callback that can be provided at construction
        std::function<void()> _stop_callback;
        // jthread to handle running the callable, created upon task start
        std::optional<std::jthread> _thread;
        // Storage for the stop_callback if registered
        std::optional<std::stop_callback<std::function<void()>>> _stop_callback_wrapper;
        // Completion event which enables waiting
        shm::event _completion_event;
        // Exception capture
       std::exception_ptr _exception;
        // Result storage
        std::conditional_t<std::is_void_v<R>, std::monostate, std::optional<R>> _result;

        // Atomic uint for generating task ids
        static inline std::atomic<uint32_t> _id_gen{0};
        // This instance id
        tid _id = _id_gen++;
    };
}
