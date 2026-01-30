#pragma once

/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 themalwareman

    shim — small, header-only C++ utilities
    https://github.com/themalwareman/shim

    shm::buffer - a minimal owning array for trivially copyable types,
                    essentially a unique_ptr to a dynamically allocated array
                    that also tracks its size. Designed for raw data storage
                    where the overhead of std::vector is unnecessary.
*/

#include <cstddef>
#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <span>
#include <cassert>
#include <algorithm>

namespace shm {

    /*
        Notes:

        We require that the template type T is:

        Trivially copyable - we can just copy the bytes to re-locate the type
        Trivially destructible - freeing the underlying memory is enough to delete the type
        Not a reference type - storing references wouldn't make sense

        Trivially copyable requires trivially destructible, but it's nice to be explicit for clarity.
        The same goes for not a reference type, this is guaranteed by trivially copyable as it implies
        the type can be copied using a simple memcpy, but references don't have addresses so this makes
        no sense. So although it's guaranteed by the single constraint I want to add it to the required
        clauses for clarity as things like these are not always obvious.

        The original goal was something akin to the proposed std::dynarray but there's a few ways I
        wanted this class to differ. Dynarray promised construction which I do not want to do, this
        class I want to act like a raw buffer where the contents need no construction/initialization
        and is essentially a dynamic buffer/array of raw data types like POD structs or scalar types.

        With that in mind I will roughly mimic this where I think it is useful:
        https://www.cs.helsinki.fi/group/boi2016/doc/cppreference/reference/en.cppreference.com/w/cpp/container/dynarray/dynarray.html

        The idea is that the proposal for the standard is probably fairly sensible as a baseline.
    */
    template<typename T>
    requires std::is_trivially_copyable_v<T> &&
        std::is_trivially_destructible_v<T> &&
        (not std::is_reference_v<T>)
    class buffer {
    public:
        // Try and stay consistent with stl
        using value_type = T;
        using size_type  = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using iterator = value_type*;
        using const_iterator = const value_type*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        /*
            Constructors

            Dynarray did not provide a default constructor but for the ability to predeclare
            a variable for scoping purposes I think its helpful, and it's semantically identical
            to initializing a zero length buffer using the explicit constructor.

            // Constructors
            buffer()
            explicit buffer(size_type count)
            explicit buffer(size_type count, const value_type& value)
            template <typename U>
            buffer(std::initializer_list<U> init) requires std::is_convertible_v<U, T>

            // Copy construct & deleted copy assign
            explicit buffer(const buffer& other)
            buffer& operator=(const buffer&) = delete;

            // Move construct and assign
            constexpr buffer(buffer&& other) noexcept
            buffer& operator=(buffer&& other) noexcept

            // Destructor
            ~buffer() noexcept

            // Element Access
            [[nodiscard]] constexpr reference operator[](size_type i) noexcept
            [[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept
            [[nodiscard]] constexpr reference at(size_type i)
            [[nodiscard]] constexpr const_reference at(size_type i) const
            [[nodiscard]] constexpr pointer data() noexcept
            [[nodiscard]] constexpr const_pointer data() const noexcept
            [[nodiscard]] constexpr reference front() noexcept
            [[nodiscard]] constexpr const_reference front() const noexcept
            [[nodiscard]] constexpr reference back() noexcept
            [[nodiscard]] constexpr const_reference back() const noexcept

            [[nodiscard]] constexpr std::span<value_type> span() noexcept
            [[nodiscard]] constexpr std::span<const value_type> span() const noexcept
            [[nodiscard]] constexpr std::span<value_type> first(size_type count) noexcept
            [[nodiscard]] constexpr std::span<const value_type> first(size_type count) const noexcept
            [[nodiscard]] constexpr std::span<value_type> last(size_type count) noexcept
            [[nodiscard]] constexpr std::span<const value_type> last(size_type count) const noexcept
            [[nodiscard]] constexpr std::span<value_type> subspan(size_type offset) noexcept
            [[nodiscard]] constexpr std::span<const value_type> subspan(size_type offset) const noexcept
            [[nodiscard]] constexpr std::span<value_type> subspan(size_type offset, size_type count) noexcept
            [[nodiscard]] constexpr std::span<const value_type> subspan(size_type offset, size_type count) const noexcept

            // Capacity
            [[nodiscard]] constexpr size_type size() const noexcept
            [[nodiscard]] constexpr bool empty() const noexcept

            // Iterator Support
            constexpr iterator begin() noexcept
            constexpr iterator end() noexcept
            constexpr const_iterator begin() const noexcept
            constexpr const_iterator end() const noexcept
            constexpr const_iterator cbegin() const noexcept
            constexpr const_iterator cend() const noexcept
            constexpr reverse_iterator rbegin() noexcept
            constexpr reverse_iterator rend() noexcept
            constexpr const_reverse_iterator rbegin() const noexcept
            constexpr const_reverse_iterator rend() const noexcept
            constexpr const_reverse_iterator crbegin() const noexcept
            constexpr const_reverse_iterator crend() const noexcept

            // Modifiers
            constexpr void fill(const value_type& value) noexcept

            // Implicit span conversion
            constexpr operator std::span<value_type>() noexcept
            constexpr operator std::span<const value_type>() const noexcept

            // Comparison
            friend bool operator==(const buffer& lhs, const buffer& rhs) noexcept
            friend bool operator!=(const buffer& lhs, const buffer& rhs) noexcept

        */

        // Default constructor
        constexpr buffer() : _size(0), _data(nullptr) {}

        explicit buffer(size_type count) :
            _size(count),
            _data(count ? static_cast<pointer>(operator new(sizeof(value_type) * count, std::align_val_t{alignof(T)})) : nullptr) {}

        explicit buffer(size_type count, const value_type& value)
            : buffer(count) // Delegate
        {
            fill(value);
        }

        template <typename U>
        buffer(std::initializer_list<U> init) requires std::is_convertible_v<U, T> :
            _size(init.size()),
            _data(init.size() ? static_cast<pointer>(operator new(sizeof(value_type) * init.size(), std::align_val_t{alignof(T)})) : nullptr)
        {
            auto* src = init.begin();
            for (size_type i = 0; i < _size; i++) {
                _data[i] = static_cast<T>(src[i]);
            }
        }

        /*
            Copy construct & deleted copy assign
        */

        // Explicit copy constructor to avoid accidental allocations and mishaps
        explicit buffer(const buffer& other) :
            _size(other._size),
            _data(other._size ? static_cast<pointer>(operator new(sizeof(value_type) * other._size, std::align_val_t{alignof(T)})) : nullptr)
        {
            if (_size > 0) {
                std::memcpy(_data, other._data, sizeof(T) * _size);
            }
        }

        /*
            Delete copy-assign. It should never be what's intended, and they can either
            move or use the explicit copy construct.
        */
        buffer& operator=(const buffer&) = delete;

        /*
            Move construct & assign
        */

        constexpr buffer(buffer&& other) noexcept : _size(other._size), _data(other._data)
        {
            // No need for self assignment check in move constructor
            other._data = nullptr;
            other._size = 0;
        }

        // Move assign
        buffer& operator=(buffer&& other) noexcept
        {
            // Check for self move assign
            if (this != &other)
            {
                /*
                    Rather than handoff memory for val to clean up delete it now
                    this reduces the scope for how long the memory is allocated in
                    case the moved from object has a large scope.
                */
                if (_data) {
                    operator delete(_data, std::align_val_t{alignof(T)});
                }
                _size = other._size;
                _data = other._data;
                other._data = nullptr;
                other._size = 0;
            }
            return *this;
        }

        /*
            Destructor
        */
        ~buffer() noexcept
        {
            if (_data) {
                operator delete(_data, std::align_val_t{alignof(T)});
            }
        }

        /*
            Element Access
        */
        [[nodiscard]] constexpr reference operator[](size_type i) noexcept {
            assert(i < _size && "dynarray::operator[] index out of bounds");
            return data()[i];
        }

        [[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept {
            assert(i < _size && "dynarray::operator[] index out of bounds");
            return data()[i];
        }

        [[nodiscard]] constexpr reference at(size_type i) {
            if (i >= size()) throw std::out_of_range("dynarray::at() index out of bounds");
            return data()[i];
        }

        [[nodiscard]] constexpr const_reference at(size_type i) const {
            if (i >= size()) throw std::out_of_range("dynarray::at() index out of bounds");
            return data()[i];
        }

        [[nodiscard]] constexpr pointer data() noexcept { return _data; }
        [[nodiscard]] constexpr const_pointer data() const noexcept { return _data; }

        [[nodiscard]] constexpr reference front() noexcept {
            assert(_size > 0 && "buffer::front() on empty buffer");
            return *data();
        }
        [[nodiscard]] constexpr const_reference front() const noexcept {
            assert(_size > 0 && "buffer::front() on empty buffer");
            return *data();
        }

        [[nodiscard]] constexpr reference back() noexcept {
            assert(_size > 0 && "buffer::back() on empty buffer");
            return *(data() + size() - 1);
        }
        [[nodiscard]] constexpr const_reference back() const noexcept {
            assert(_size > 0 && "buffer::back() on empty buffer");
            return *(data() + size() - 1);
        }

        [[nodiscard]] constexpr std::span<value_type> span() noexcept {
            return std::span<T>(data(), size());
        }

        [[nodiscard]] constexpr std::span<const value_type> span() const noexcept {
            return std::span<const T>(data(), size());
        }

        [[nodiscard]] constexpr std::span<value_type> first(size_type count) noexcept
        {
            assert(count <= size() && "buffer::first(count) out of range");
            return std::span<value_type>(data(), count);
        }

        [[nodiscard]] constexpr std::span<const value_type> first(size_type count) const noexcept
        {
            assert(count <= size() && "buffer::first(count) out of range");
            return std::span<const value_type>(data(), count);
        }

        [[nodiscard]] constexpr std::span<value_type> last(size_type count) noexcept
        {
            assert(count <= size() && "buffer::last(count) out of range");
            return std::span<value_type>(data() + (size() - count), count);
        }

        [[nodiscard]] constexpr std::span<const value_type> last(size_type count) const noexcept
        {
            assert(count <= size() && "buffer::last(count) out of range");
            return std::span<const value_type>(data() + (size() - count), count);
        }

        [[nodiscard]] constexpr std::span<value_type> subspan(size_type offset) noexcept
        {
            assert(offset <= size() && "buffer::subspan(offset) out of range");
            return std::span<value_type>(data() + offset, size() - offset);
        }

        [[nodiscard]] constexpr std::span<const value_type> subspan(size_type offset) const noexcept
        {
            assert(offset <= size() && "buffer::subspan(offset) out of range");
            return std::span<const value_type>(data() + offset, size() - offset);
        }

        [[nodiscard]] constexpr std::span<value_type> subspan(size_type offset, size_type count) noexcept
        {
            assert(offset <= size() && "buffer::subspan(offset) out of range");
            assert(offset + count <= size() && "buffer::subspan(offset, count) out of range");
            return std::span<value_type>(data() + offset, count);
        }

        [[nodiscard]] constexpr std::span<const value_type> subspan(size_type offset, size_type count) const noexcept
        {
            assert(offset <= size() && "buffer::subspan(offset) out of range");
            assert(offset + count <= size() && "buffer::subspan(offset, count) out of range");
            return std::span<const value_type>(data() + offset, count);
        }

        /*
            Capacity
        */
        [[nodiscard]] constexpr size_type size() const noexcept { return _size; }
        [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

        /*
            Iterator support
        */
        constexpr iterator begin() noexcept { return data(); }
        constexpr iterator end() noexcept { return data() + size(); }

        constexpr const_iterator begin() const noexcept { return data(); }
        constexpr const_iterator end() const noexcept { return data() + size(); }

        constexpr const_iterator cbegin() const noexcept { return data(); }
        constexpr const_iterator cend() const noexcept { return data() + size(); }

        constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

        constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
        constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        /*
            Modifiers
        */
        constexpr void fill(const value_type& value) noexcept {
            std::fill(data(), data() + size(), value);
        }

        /*
            Make it implicitly convertible to a span for compatability
        */

        constexpr operator std::span<value_type>() noexcept {
            return std::span<T>(data(), size());
        }

        constexpr operator std::span<const value_type>() const noexcept {
            return std::span<const T>(data(), size());
        }

        /*
            Comparison
        */
        friend bool operator==(const buffer& lhs, const buffer& rhs) noexcept {
            if (lhs._size != rhs._size) return false;
            // Safe because T is trivially copyable
            return lhs._size == 0 || std::memcmp(lhs._data, rhs._data, lhs._size * sizeof(value_type)) == 0;
        }

        friend bool operator!=(const buffer& lhs, const buffer& rhs) noexcept {
            return !(lhs == rhs);
        }

    private:
        size_type _size;
        T* _data;
    };

}