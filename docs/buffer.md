# shm::buffer<T>

## :bookmark: Overview

A minimal owning array for trivially copyable types, essentially a `std::unique_ptr<T[]>` to a dynamically allocated array that also tracks its size. Designed for raw data storage where the overhead of `std::vector` is unnecessary.

---

## :rocket: Quick Example

```cpp
#include "shm/buffer.hpp"

// Allocate buffer that holds 1024 uint8_ts
shm::buffer<uint8_t> data(1024);

// Can be passed around to be used as a raw buffer
sock.recv(data);

// Implicitly convertible to std::span
std::span<uint8_t> view = data;
```

---

## :balance_scale: Why Not `std::vector` or `std::unique_ptr<T[]>`?

`std::vector` isn't ideal as a raw buffer, it wants to value initialize everything which is slow and the modifying the contents of the vector via `.data()` is sort of an abuse of the api. It's safe for trivial types like a `std::vector<uint8_t>` but that doesn't make it nice. `std::unique_ptr<T[]>` is great except for that fact that you have to send the size of the array everywhere with it, and it doesn't have easy conversion to a `std::span` or iterators for a range based for loop. Plus, anytime you want element access you have a `->` or a `.get()`. `shm::buffer` is sort of what I think `std::dynarray` was supposed to be. I've always found a need for something like this so I made one. I basically just want a raw memory allocation I can't forget to free that is easy to pass around and easy to access.

---

## :package: API

Just a quick note to say that I have added clang style stl docs to each header, so if you want the full api it's easier to just look at the source e.g.

```cpp
/*
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

...
*/
```

### Constructors

You can create an empty buffer, initialize by size, with or without a value. I've also added support for creation from an initializer list.

```cpp
 buffer()
explicit buffer(size_type count)
explicit buffer(size_type count, const value_type& value)
template <typename U>
buffer(std::initializer_list<U> init) requires std::is_convertible_v<U, T>
```

---

### Copy & Move

It's explicitly copy-constructable in case you need to duplicate the buffer, but copy assign is deleted. And then its obviously movable in case you need to transfer ownership across function boundaries.

```cpp
explicit buffer(const buffer& other)
buffer& operator=(const buffer&) = delete;

constexpr buffer(buffer&& other) noexcept
buffer& operator=(buffer&& other) noexcept
```

---

### Element Access

For element access it has unchecked `operator[]`, bounds checked `.at()`, then similar to a `std::vector` it has `front()`, `back()` & `data()`.

Its also convertible to `std::span`, but also provides `.first(n)`, `.last(n)` & `.subspan(offset [, count])`. It also has an explicit `.span()` method.

---

### Capacity

```cpp
size_type size() const;
bool empty() const;
```

---

### Iterators

Supports standard iteration:

```cpp
begin(), end()
rbegin(), rend()
```

---

### Modifiers

```cpp
void fill(const T& value);
```

---

## :warning: Notes

* Memory is **not initialised** unless explicitly filled
* Internally uses `operator new` / `operator delete`
* No resizing or reallocation support
* Copy assignment is intentionally disabled
* Comparisons use `std::memcmp` (safe due to type constraints)

---

## :exclamation: Type Requirements

`T` must be:

* Trivially copyable
* Trivially destructible
* Not a reference type

This allows:

* Raw byte-wise copying (`memcpy`)
* No construction/destruction overhead
* Safe use of low-level memory operations
