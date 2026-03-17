# shm::event

## :bookmark: Overview

A lightweight, thread-safe event primitive for signaling between threads.

Provides Windows-style manual and auto-reset semantics without the boilerplate of `std::condition_variable`.

---

## :rocket: Quick Example

```cpp
#include "shim/event.h"

shm::event evt;

std::thread worker([&] {
    // Do work...
    // Signal event
    evt.set();
});

// Wait for worker to signal event
evt.wait();
```

---

## :gear: Modes

`shm::event` supports two modes:

* **manual_reset**

  * Stays signaled until `reset()` is called
  * Wakes all waiting threads

* **auto_reset**

  * Automatically resets after releasing a single waiting thread
  * Wakes one thread per `set()`

---

## :brain: When to Use

Use this when you want:

* A simple thread signaling mechanism
* Less boilerplate than `std::condition_variable`
* A reusable "event" style primitive

Typical use cases:

* Worker -> main thread completion signals
* Thread coordination without shared predicates
* One-shot or repeatable notifications

---

## :package: API

### Constructor

```cpp
explicit event(mode mode = mode::manual_reset, bool signaled = false);
```

* `mode` — manual or auto reset behaviour
* `signaled` — initial state

---

### Waiting

```cpp
void wait();
```

Block until the event is signaled.

```cpp
bool wait_for(duration);
bool wait_until(time_point);
```

Wait with timeout. Returns `true` if signaled.

```cpp
bool try_wait();
```

Non-blocking check. Preserves auto-reset semantics.

---

### Signaling

```cpp
void set();
```

Signal the event.

* Wakes **one thread** in auto-reset mode
* Wakes **all threads** in manual-reset mode

```cpp
void reset();
```

Reset the event to a non-signaled state.

---

## :warning: Notes

* Thread-safe: internal mutex + condition variable
* Non-copyable and non-movable
* Auto-reset semantics are applied **after a successful wait**
* `try_wait()` will also consume the signal in auto-reset mode

---

## :bulb: Design Notes

This is intentionally a small abstraction over `std::condition_variable`:

* Combines state + synchronization into one object
* Avoids repeated predicate boilerplate
* Prioritises clarity over flexibility

