// Grab buffer class from shim
#include <shim/buffer.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <span>

// A function that operates on a non-owning view
void scale(std::span<int> values, int factor) {
    for (int& v : values)
        v *= factor;
}

// A function that only needs read-only access
int sum(std::span<const int> values) {
    return std::accumulate(values.begin(), values.end(), 0);
}

/*
    For a more in-depth explanation on why shm::buffer exists please
    go read the header file comments :)
*/
int main() {
    // ------------------------------------------------------------------
    // 1. Owning, fixed-size allocation
    // ------------------------------------------------------------------
    shm::buffer<int> buf(8);
    assert(buf.size() == 8);

    // ------------------------------------------------------------------
    // 2. Iterator support (works with standard algorithms)
    // ------------------------------------------------------------------
    std::iota(buf.begin(), buf.end(), 1); // 1, 2, 3, ...

    std::cout << "initial values: ";
    for (int v : buf) {                  // range-for via iterators
        std::cout << v << " ";
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 3. Implicit conversion to std::span (mutable)
    // ------------------------------------------------------------------
    scale(buf, 10);                       // converts to std::span<int>

    // ------------------------------------------------------------------
    // 4. Implicit conversion to std::span (const)
    // ------------------------------------------------------------------
    int total = sum(buf);                 // converts to std::span<const int>
    std::cout << "sum after scaling: " << total << "\n";

    // ------------------------------------------------------------------
    // 5. Explicit view creation (sometimes clearer)
    // ------------------------------------------------------------------
    std::span<int> view = buf;
    view[0] = 999;

    std::cout << "after modifying through span: ";
    for (int v : buf) {
        std::cout << v << " ";
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 6. Intent: no resizing, no capacity, no reallocation
    // ------------------------------------------------------------------
    // buf.push_back(...);   // does not exist
    // buf.resize(...);      // does not exist
    //
    // The size is fixed at construction and explicit by design.

    // ------------------------------------------------------------------
    // 7. Only trivial types
    // ------------------------------------------------------------------
    // buffer is designed to work with trivially copyable, trivially destructible types.
    // It is intentionally minimal: no methods beyond ownership and span access.
    // This allows allocation of raw, uninitialized memory (like a unique_ptr
    // that also tracks the size of the allocation).
    //
    // The following line would fail to compile if uncommented, because std::string
    // is non-trivial:
    //
    // shm::buffer<std::string> stringBuf(5);
    //
    // This illustrates that buffer is **not** a general container — it is a low-level
    // utility for fixed-size, trivially constructible types.

    // ------------------------------------------------------------------
    // 8. Trivial structs
    // ------------------------------------------------------------------
    // buffer works perfectly with user-defined structs that are trivially copyable
    // and trivially destructible. This is a common pattern for raw storage of
    // plain data, similar to arrays of POD types.

    struct Point {
        float x;
        float y;
        float z;
    };

    shm::buffer<Point> points(5);  // allocates space for 5 Points

    // Initialize via iterators
    for (std::size_t i = 0; i < points.size(); ++i) {
        points[i] = Point{float(i), float(i * 2), float(i * 3)};
    }

    // Read via span
    std::span<const Point> point_view = points;
    for (const auto& p : point_view) {
        std::cout << "Point(" << p.x << ", " << p.y << ", " << p.z << ")\n";
    }


    return 0;
}
