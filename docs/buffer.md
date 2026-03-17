# shm::buffer<T>

## :bookmark: Overview

A minimal owning array for trivially copyable types.

`shm::buffer<T>` is essentially a `std::unique_ptr<T[]>` that also tracks its size, designed for raw data storage where the overhead and semantics of `std::vector` are unnecessary.

---

## :rocket: Quick Example

```cpp
#include "shm/buffer.hpp"

shm::buffer<uint8_t> data(1024);

// Write raw data
data[0] = 42;

// View as span
std::span<uint8_t> view = data;
```

---

## :brain: When to Use

Use this when you want:

* A simple owning buffer of raw data
* No default/value initialisation overhead
* Contiguous memory with a known size
* Easy interop with APIs expecting `std::span` or pointers

Typical use cases:

* Binary data / file buffers
* Networking / serialization
* POD structs or plain numeric arrays
* Performance-sensitive allocations

---

## :balance_scale: Why Not `std::vector`?

`std::vector` is great—but it does more than you may need:

* Always value-initialises elements
* Supports resizing, capacity growth, etc.
* Implies element "lifetime" semantics

`shm::buffer<T>` is intentionally simpler:

* No resizing
* No capacity management
* No implicit initialisation
* Just raw, owned memory + size

---

## :package: API

### Construction

```cpp
buffer();
explicit buffer(size_type count);
buffer(size_type count, const T& value);
buffer(std::initializer_list<U>);
```

* `count` allocates raw storage (uninitialised)
* Optional fill constructor initialises values
* Initialiser list copies values into the buffer

---

### Copy & Move

```cpp
buffer(const buffer& other);
buffer(buffer&& other) noexcept;
buffer& operator=(buffer&& other) noexcept;
```

* Copy construction performs a full copy
* Copy assignment is **disabled**
* Move is cheap (pointer + size transfer)

---

### Element Access

```cpp
T& operator[](size_type i);
T& at(size_type i);
T* data();
T& front();
T& back();
```

* `operator[]` uses assertions for bounds checking
* `at()` throws on out-of-range access

---

### Views (Span Support)

```cpp
std::span<T> span();
std::span<const T> span() const;
```

Also provides:

* `first(n)`
* `last(n)`
* `subspan(offset[, count])`

Implicit conversion to `std::span` is supported:

```cpp
std::span<uint8_t> view = buffer;
```

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

---

## :bulb: Design Notes

This is intentionally a low-level utility:

* Prioritises performance and simplicity
* Avoids unnecessary abstractions
* Behaves more like a raw buffer than a container

It’s closest in spirit to the proposed `std::dynarray`, but with stricter constraints and no element construction.

If you need resizing, complex ownership semantics, or non-trivial types, `std::vector` is likely the better choice.
