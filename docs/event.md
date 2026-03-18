# shm::event

## :bookmark: Overview

A lightweight, thread-safe manual- or auto-reset event for signaling between threads, providing set, reset, and wait semantics without the boilerplate of condition variables.

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


