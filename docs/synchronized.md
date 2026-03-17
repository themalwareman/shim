# shm::synchronized<T>

## :bookmark: Overview

A small utility that combines a value and a mutex into a single abstraction, providing safe, scoped access to shared data.

Designed to reduce the chances of forgetting to lock a mutex, while keeping access patterns explicit and easy to read.

---

## :rocket: Quick Example

```cpp
#include "shm/synchronized.hpp"

shm::synchronized<int> value(0);

// Safe write
value.with_exclusive([](int& v) {
    v += 1;
});

// Safe read
int result = value.with_shared([](const int& v) {
    return v;
});
```

---

## :brain: When to Use

Use this when you want:

* A simple way to pair data with its mutex
* To avoid manually managing `std::mutex` / `std::shared_mutex`
* Safer access patterns in multithreaded code
* Clear, scoped locking

Typical use cases:

* Shared state between threads
* Protecting small pieces of mutable data
* Replacing "mutex + variable" pairs scattered through code

---

## :gear: Access Patterns

### 1. Callable Access (Recommended)

```cpp
value.with_exclusive([](T& v) { /* modify */ });
value.with_shared([](const T& v) { /* read */ });
```

* Locks are automatically acquired and released
* Prevents references from escaping the critical section
* Returns values by copy

---

### 2. Guard-Based Access (Legacy)

```cpp
auto guard = value.lock();
guard->do_something();

auto shared = value.lock_shared();
auto x = shared->get();
```

* RAII-style locking
* Allows direct access via `->` / `*`
* More flexible, but easier to misuse

---

## :package: API

### Construction

```cpp
synchronized();
template <typename... Args>
explicit synchronized(Args&&... args);
```

Constructs the underlying value in-place.

---

### Callable Access

```cpp
template <typename F>
auto with_exclusive(F&& f);
```

* Acquires exclusive lock
* Invokes `f(T&)`

```cpp
template <typename F>
auto with_shared(F&& f) const;
```

* Acquires shared lock
* Invokes `f(const T&)`

---

### Guard Access

```cpp
exclusive_guard lock();
shared_guard lock_shared() const;
```

Returns RAII guards that hold the lock for the lifetime of the object.

---

## :warning: Notes

* Internally uses `std::shared_mutex`
* Non-copyable and non-movable
* `with_*` methods return by value to prevent leaking references
* Lambdas must take references (`T&` / `const T&`) to avoid unintended copies

---

## :exclamation: Type Requirements

`T` must:

* Not be a reference
* Not be `const`
* Not be `void`

These restrictions help prevent misuse and ensure meaningful synchronization.

---

## :bulb: Design Notes

This is intentionally a minimal abstraction:

* Encourages correct locking by construction
* Keeps access explicit rather than implicit
* Avoids exposing the mutex directly

Two access styles are provided:

* **Callable (`with_*`)** for safety and simplicity
* **Guards (`lock`)** for flexibility when needed

If you require fine-grained control over locking behaviour, using `std::shared_mutex` directly may still be more appropriate.
