#pragma once

/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 themalwareman

    shim — small, header-only C++ utilities
    https://github.com/themalwareman/shim

    shm::task — a cooperative task built on jthread with
            stop token support and enables deferred launch.

    shm::task_group — a lifetime manager for multiple tasks.
*/

#include "event.h"

#include <concepts>
#include <thread>
#include <optional>
#include <type_traits>
#include <functional>
#include <list>

namespace shm {

    class task {

    public:

        enum class launch_policy { immediate, deferred };

        using tid = uint32_t;

        [[nodiscard]] tid id() const { return _id; }

        /*
            // Constructors
            template<typename F>
            requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>>
            explicit task(F&& callable, launch_policy policy = launch_policy::immediate)

            template<typename F, typename S>
            requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>> &&
                std::invocable<S> && std::copy_constructible<std::decay_t<S>>
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
            void stop()
            [[nodiscard]] bool started() const

            // Waiting
            void wait()
            template <typename Rep, typename Period>
            [[nodiscard]] bool wait_for(const std::chrono::duration<Rep, Period>& timeout)
            template <typename Clock, typename Duration>
            [[nodiscard]] bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point)
            [[nodiscard]] bool try_wait()

        */

        /*
         * I want to force any users to have to take a stop_token so that willfully
         * ignoring it is at least a choice rather than likely a mistake. We also
         * for now require the callable to be copy constructable because for now
         * we use a std::function to store it and that's a requirement.
         */
        template<typename F>
        requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>>
        explicit task(F&& callable, launch_policy policy = launch_policy::immediate) : _callable(std::forward<F>(callable))
        {
            // Are we launching the work right away?
            if (policy == launch_policy::immediate) {
                this->start();
            }
        }

        template<typename F, typename S>
        requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>> &&
            std::invocable<S> && std::copy_constructible<std::decay_t<S>>
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
                jthread takes this to set the event, to prevent member destruction ordering issues
                we wait on the join in our own destructor first to guarantee the thread has exited.
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

        void stop() {
            request_stop();
            _completion_event.wait();
        }

        [[nodiscard]] bool started() const {
            return _thread.has_value();
        }

        /*
            Waiting
        */

        void wait() {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            _completion_event.wait();
        }

        template <typename Rep, typename Period>
        [[nodiscard]] bool wait_for(const std::chrono::duration<Rep, Period>& timeout) {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            return _completion_event.wait_for(timeout);
        }

        template <typename Clock, typename Duration>
        [[nodiscard]] bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point) {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            return _completion_event.wait_until(time_point);
        }

        [[nodiscard]] bool try_wait() {
            if (not _thread.has_value()) {
                throw std::runtime_error("cannot wait on unstarted task");
            }
            return _completion_event.try_wait();
        }

    private:

        // Centralised code path
        void run_task(const std::stop_token& token) {

            /*
             * We could grab the stop token from the jthread here, but it's a bit janky
             * so we'll just get the jthread creator to pass it through to us.
             */

            // Invoke the callable passing the stop_token
            _callable(token);

            // Work is done
            _completion_event.set();
        }

    private:
        // Callable to invoke upon task start
        std::function<void(std::stop_token)> _callable;
        // Optional stop callback that can be provided at construction
        std::function<void()> _stop_callback;
        // jthread to handle running the callable, created upon task start
        std::optional<std::jthread> _thread;
        // Storage for the stop_callback if registered
        std::optional<std::stop_callback<std::function<void()>>> _stop_callback_wrapper;
        // Completion event which enables waiting
        shm::event _completion_event;

        // Atomic uint for generating task ids
        static inline std::atomic<uint32_t> _id_gen{0};
        // This instance id
        tid _id = _id_gen++;
    };


    class task_group {
    public:

        /*
            // Constructors
            task_group() = default;

            // Copy construct and copy assign are deleted
            task_group(const task_group&) = delete;
            task_group& operator=(const task_group&) = delete;

            // Move construct and move assign are also deleted
            task_group(task_group&&) = delete;
            task_group& operator=(task_group&&) = delete;

            // Destructor
            ~task_group()

            // Tasks
            template<typename F>
            requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>>
            task& add(F&& callable, task::launch_policy policy = task::launch_policy::immediate)

            template<typename F, typename S>
            requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>> &&
                std::invocable<S> && std::copy_constructible<std::decay_t<S>>
            task& add(F&& callable, S&& stop_callback, task::launch_policy policy = task::launch_policy::immediate)

            // Group
            void start()
            void request_stop()
            void stop()

            // Waiting
            void wait()
            template <typename Rep, typename Period>
            [[nodiscard]] bool wait_for(const std::chrono::duration<Rep, Period>& timeout)
            template <typename Clock, typename Duration>
            [[nodiscard]] bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point)
            [[nodiscard]] bool try_wait()

        */


        task_group() = default;

        // Non-copyable
        task_group(const task_group&) = delete;
        task_group& operator=(const task_group&) = delete;

        // Non-movable
        task_group(task_group&&) = delete;
        task_group& operator=(task_group&&) = delete;

        // Ensure we stop all tasks before we destruct
        ~task_group()
        {
            request_stop();
            wait();
        }

        /*
            Tasks
        */

        template<typename F>
        requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>>
        task& add(F&& callable, task::launch_policy policy = task::launch_policy::immediate)
        {
            return _tasks.emplace_back(std::forward<F>(callable), policy);
        }

        template<typename F, typename S>
        requires std::invocable<F, std::stop_token> && std::copy_constructible<std::decay_t<F>> &&
            std::invocable<S> && std::copy_constructible<std::decay_t<S>>
        task& add(F&& callable, S&& stop_callback, task::launch_policy policy = task::launch_policy::immediate)
        {
            return _tasks.emplace_back(std::forward<F>(callable), std::forward<S>(stop_callback), policy);
        }

        /*
            Group
        */

        void start() {
            for (auto& t : _tasks) {
                if (not t.started()) {
                    t.start();
                }
            }
        }

        void request_stop() {
            for (auto& t : _tasks) {
                if (t.started()) {
                    t.request_stop();
                }
            }
        }

        void stop() {
            request_stop();
            wait();
        }

        /*
            Waiting
        */

        void wait() {
            for (auto& t : _tasks) {
                if (t.started()) {
                    t.wait();
                }
            }
        }

        template <typename Rep, typename Period>
        [[nodiscard]] bool wait_for(const std::chrono::duration<Rep, Period>& timeout) {
            return wait_until(std::chrono::steady_clock::now() + timeout);
        }

        template <typename Clock, typename Duration>
        [[nodiscard]] bool wait_until(const std::chrono::time_point<Clock, Duration>& time_point) {
            bool retval = true;

            for (auto& t : _tasks) {
                if (t.started() && not t.wait_until(time_point)) {
                    retval = false;
                    break;
                }
            }

            return retval;
        }

        [[nodiscard]] bool try_wait() {
            bool retval = true;

            for (auto& t : _tasks) {
                if (t.started() && not t.try_wait()) {
                    retval = false;
                    break;
                }
            }

            return retval;
        }

    private:
        std::list<task> _tasks;
    };
}
