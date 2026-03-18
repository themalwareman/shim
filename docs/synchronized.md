# shm::synchronized<T>

## :bookmark: Overview

A small utility for safe, scoped access to shared data, combining a mutex and an object into a single abstraction.

Makes it impossible to forget to lock the mutex, but makes keeps access simple, explicit and easy to follow.

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
auto x = shared->access_value();
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

The constructor allows you to construct a value in place, say if you wanted to guard a map that started with some pre-filled values.

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
