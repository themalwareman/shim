
#include <catch2/catch_amalgamated.hpp>

#include <shim/buffer.h>

#include <vector>
#include <string>

#include <cstdint>
#include <numeric>
#include <type_traits>
#include <algorithm>


struct TrivialPod {
    int x;
    float y;
    bool operator==(const TrivialPod&) const = default;
};

struct AlignedPod {
    alignas(16) double value;
    bool operator==(const AlignedPod&) const = default;
};


/*
    Buffer Tests
*/

TEST_CASE("Static constraints", "[buffer][constraints]")
{
    /*
        Test Cases:

        - Buffer is not copy-assignable
        - Buffer is movable
    */

    SECTION("Copy assignment is deleted")
    {
        STATIC_CHECK_FALSE(std::is_copy_assignable_v<shm::buffer<int>>);
    }

    SECTION("Move operations exist")
    {
        STATIC_CHECK(std::is_move_constructible_v<shm::buffer<int>>);
        STATIC_CHECK(std::is_move_assignable_v<shm::buffer<int>>);
    }
}


TEST_CASE("Constructors", "[buffer][constructors]")
{
    /*
        Test Cases:

        - Default constructor
        - Count constructor
        - Count with 0 behaves like default
        - Count with value
        - Count with value 0 count
        - Initialiser list
        - Initialiser list implicit conversion U->T
        - Struct value
        - Aligned struct value
    */

    SECTION("Default constructor")
    {
        shm::buffer<int> buf;

        CHECK(buf.size() == 0);
        CHECK(buf.empty());
        CHECK(buf.data() == nullptr);
        CHECK(buf.begin() == buf.end());
    }

    SECTION("Count constructor")
    {
        shm::buffer<int> buf(16);
        CHECK(buf.size() == 16);
        CHECK_FALSE(buf.empty());
        CHECK(buf.data() != nullptr);
        CHECK(buf.begin() != buf.end());
    }

    SECTION("Count with 0 behaves like default")
    {
        shm::buffer<int> buf(0);
        CHECK(buf.size() == 0);
        CHECK(buf.empty());
        CHECK(buf.data() == nullptr);
        CHECK(buf.begin() == buf.end());
    }

    SECTION("Count with value")
    {
        shm::buffer<int> buf(8, 42);
        CHECK(buf.size() == 8);
        CHECK_FALSE(buf.empty());
        CHECK(buf.data() != nullptr);
        CHECK(buf.begin() != buf.end());

        for (std::size_t i = 0; i < buf.size(); ++i) {
            CHECK(buf[i] == 42);
        }
    }

    SECTION("Count with value 0 count")
    {
        shm::buffer<int> buf(0, 42);
        CHECK(buf.size() == 0);
        CHECK(buf.empty());
        CHECK(buf.data() == nullptr);
        CHECK(buf.begin() == buf.end());
    }

    SECTION("Initialiser list")
    {
        shm::buffer<int> buf({1, 2, 3, 4});
        CHECK(buf.size() == 4);
        CHECK_FALSE(buf.empty());
        CHECK(buf.data() != nullptr);
        CHECK(buf.begin() != buf.end());
        CHECK(buf[0] == 1);
        CHECK(buf[1] == 2);
        CHECK(buf[2] == 3);
        CHECK(buf[3] == 4);
    }

    SECTION("Initialiser list implicit conversion U->T")
    {
        shm::buffer<float> buf{1, 2, 3};
        REQUIRE(buf.size() == 3);
        CHECK(buf[0] == 1.0f);
        CHECK(buf[1] == 2.0f);
        CHECK(buf[2] == 3.0f);
    }

    SECTION("Struct value")
    {
        TrivialPod pod{7, 1.5f};
        shm::buffer<TrivialPod> buf(3, pod);
        for (auto& v : buf) {
            CHECK(v == pod);
        }
    }

    SECTION("Aligned struct value")
    {
        AlignedPod pod{1.5f};
        shm::buffer<AlignedPod> buf(3, pod);
        CHECK(reinterpret_cast<std::uintptr_t>(buf.data()) % alignof(AlignedPod) == 0);
    }
}

TEST_CASE("Copy constructor", "[buffer][constructors]")
{
    /*
        Test Cases:

        - Copy has independent storage
        - Copy of empty buffer doesn't allocate
        - Copied data is identical
        - Copy is considered equal
    */
    SECTION("Copy gives own allocation")
    {
        shm::buffer<int> orig{1, 2, 3};
        shm::buffer<int> copy(orig);

        REQUIRE(copy.size() == orig.size());
        copy[0] = 999;
        CHECK(orig[0] == 1);
        CHECK(copy[0] == 999);
        CHECK(orig.data() != copy.data());
    }

    SECTION("Copy of empty buffer doesn't allocate")
    {
        shm::buffer<int> empty;
        shm::buffer<int> copy(empty);
        CHECK(copy.size() == 0);
        CHECK(copy.empty());
        CHECK(copy.data() == nullptr);
    }

    SECTION("Copied data is identical")
    {
        shm::buffer<int> orig{1, 2, 3};
        shm::buffer<int> copy(orig);
        CHECK(orig[0] == copy[0]);
        CHECK(orig[1] == copy[1]);
        CHECK(orig[2] == copy[2]);
    }

    SECTION("Copy is considered equal")
    {
        shm::buffer<int> orig{1, 2, 3};
        shm::buffer<int> copy(orig);
        CHECK(orig == copy);
    }
}

TEST_CASE("Move constructor", "[buffer][constructors]")
{
    /*
        Test Cases:

        - Source becomes empty after move
        - Empty move is safe
    */

    SECTION("Source becomes empty after move")
    {
        shm::buffer<int> src{1, 2, 3};
        int* original_ptr = src.data();

        shm::buffer<int> dst(std::move(src));

        CHECK(src.empty());
        CHECK(src.data() == nullptr);
        CHECK(dst.size() == 3);
        CHECK(dst.data() == original_ptr);
    }

    SECTION("Move of empty buffer")
    {
        shm::buffer<int> src;
        shm::buffer<int> dst(std::move(src));
        CHECK(dst.empty());
        CHECK(src.empty());
    }
}

TEST_CASE("Move assignment", "[buffer][assignment]")
{
    /*
        Test Cases:

        - Basic move assign
        - Self-move-assign is safe
    */

    SECTION("Basic move assign")
    {
        shm::buffer<int> src{10, 20, 30};
        shm::buffer<int> dst(5, 0);
        int* expected_ptr = src.data();

        dst = std::move(src);

        CHECK(dst.size() == 3);
        CHECK(dst.data() == expected_ptr);
        CHECK(dst[0] == 10);
        CHECK(src.empty());
        CHECK(src.data() == nullptr);
    }

    SECTION("Self-move-assign is safe")
    {
        shm::buffer<int> buf{1, 2, 3};
        buf = std::move(buf);
        CHECK(buf.size() == 3);
    }
}

TEST_CASE("operator[]", "[buffer][element_access]")
{
    /*
        Test Cases:

        - Read access
        - Write access
        - Const read access
    */

    shm::buffer<int> buf{10, 20, 30, 40};

    SECTION("Read access")
    {
        CHECK(buf[0] == 10);
        CHECK(buf[3] == 40);
    }

    SECTION("Write access")
    {
        buf[1] = 99;
        CHECK(buf[1] == 99);
    }

    SECTION("Const read access")
    {
        const shm::buffer<int>& cbuf = buf;
        CHECK(cbuf[2] == 30);
    }
}

TEST_CASE("at()", "[buffer][element_access]")
{
    /*
        Test Cases:

        - Valid index doesn't throw
        - Invalid index throws out-of-range
        - Invalid index throws out-of-range
        - At provides write-able access
    */

    shm::buffer<int> buf{1, 2, 3};

    SECTION("Valid index doesn't throw")
    {
        CHECK(buf.at(0) == 1);
        CHECK(buf.at(2) == 3);
    }

    SECTION("Invalid index throws out-of-range")
    {
        CHECK_THROWS_AS(buf.at(3), std::out_of_range);
        CHECK_THROWS_AS(buf.at(100), std::out_of_range);

        const shm::buffer<int>& cbuf = buf;
        CHECK_THROWS_AS(cbuf.at(3), std::out_of_range);
    }

    SECTION("At provides write-able access")
    {
        buf.at(0) = 42;
        CHECK(buf[0] == 42);
    }
}


TEST_CASE("front() and back()", "[buffer][element_access]")
{
    /*
        Test Cases:

        - front() returns first element
        - back() returns last element
        - front and back allow modification
        - front equals back on single element buffer
    */

    shm::buffer<int> buf{5, 6, 7, 8};

    SECTION("front() returns first element")
    {
        CHECK(buf.front() == 5);

        const shm::buffer<int>& cbuf = buf;
        CHECK(cbuf.front() == 5);
    }

    SECTION("back() returns last element")
    {
        CHECK(buf.back() == 8);

        const shm::buffer<int>& cbuf = buf;
        CHECK(cbuf.back()  == 8);
    }

    SECTION("front and back allow modification")
    {
        buf.front() = 100;
        buf.back()  = 200;
        CHECK(buf[0] == 100);
        CHECK(buf[3] == 200);
    }

    SECTION("front equals back on single element buffer")
    {
        shm::buffer<int> one{42};
        CHECK(one.front() == one.back());
        CHECK(&one.front() == &one.back());
    }
}


TEST_CASE("data()", "[buffer][element_access]")
{
    /*
        Test Cases:

        - Non-null for non-empty buffer
        - Null for empty buffer
        - Pointer arithmetic matches operator[]
    */

    SECTION("Non-null for non-empty buffer")
    {
        shm::buffer<int> buf(4);
        CHECK(buf.data() != nullptr);
    }

    SECTION("Null for empty buffer")
    {
        shm::buffer<int> buf;
        CHECK(buf.data() == nullptr);
    }

    SECTION("Pointer arithmetic matches operator[]")
    {
        shm::buffer<int> buf{10, 20, 30};
        CHECK(*(buf.data() + 1) == 20);
    }
}

TEST_CASE("Capacity", "[buffer][capacity]")
{
    /*
        Test Cases:

        - size() matches construction argument
        - empty() true for zero-size
        - empty() false for non-zero
    */

    SECTION("size() matches construction argument")
    {
        shm::buffer<int> buf(7);
        CHECK(buf.size() == 7);
    }

    SECTION("empty() true for zero-size")
    {
        shm::buffer<int> buf(0);
        CHECK(buf.empty());
    }

    SECTION("empty() false for non-zero")
    {
        shm::buffer<int> buf(1);
        CHECK_FALSE(buf.empty());
    }
}

TEST_CASE("span()", "[buffer][spans]")
{
    /*
        Test Cases:

        - span() covers full buffer
        - const span()
        - implicit span conversion
        - implicit const span conversion
    */

    shm::buffer<int> buf{1, 2, 3, 4, 5};

    SECTION("span() covers full buffer")
    {
        auto s = buf.span();
        CHECK(s.size() == buf.size());
        CHECK(s.data() == buf.data());
    }

    SECTION("const span()")
    {
        const shm::buffer<int>& cbuf = buf;
        auto s = cbuf.span();
        CHECK(s.size() == buf.size());
        CHECK(s.data() == buf.data());
    }

    SECTION("implicit span conversion")
    {
        std::span<int> s = buf;
        CHECK(s.size() == buf.size());
        CHECK(s.data() == buf.data());
    }

    SECTION("implicit const span conversion")
    {
        const shm::buffer<int>& cbuf = buf;
        std::span<const int> s = cbuf;
        CHECK(s.size() == cbuf.size());
    }
}

TEST_CASE("first()", "[buffer][spans]")
{
    /*
        Test Cases:

        - first(3) returns first three elements
        - first(0) is empty span
        - first(size()) covers all elements
        - const first()
    */

    shm::buffer<int> buf{1, 2, 3, 4, 5};

    SECTION("first(3) returns first three elements")
    {
        auto s = buf.first(3);
        REQUIRE(s.size() == 3);
        CHECK(s[0] == 1);
        CHECK(s[2] == 3);
    }

    SECTION("first(0) is empty span")
    {
        auto s = buf.first(0);
        CHECK(s.empty());
    }

    SECTION("first(size()) covers all elements")
    {
        auto s = buf.first(buf.size());
        CHECK(s.size() == buf.size());
    }

    SECTION("const first()")
    {
        const shm::buffer<int>& cbuf = buf;
        auto s = cbuf.first(2);
        CHECK(s.size() == 2);
        CHECK(s[0] == 1);
    }
}

TEST_CASE("last()", "[buffer][spans]")
{
    /*
        Test Cases:

        - last(2) returns last two elements
        - last(0) is empty span
        - last(size()) covers all
        - const last()
    */

    shm::buffer<int> buf{1, 2, 3, 4, 5};

    SECTION("last(2) returns last two elements")
    {
        auto s = buf.last(2);
        REQUIRE(s.size() == 2);
        CHECK(s[0] == 4);
        CHECK(s[1] == 5);
    }

    SECTION("last(0) is empty span")
    {
        CHECK(buf.last(0).empty());
    }

    SECTION("last(size()) covers all")
    {
        auto s = buf.last(buf.size());
        CHECK(s.size() == buf.size());
        CHECK(s.data() == buf.data());
    }

    SECTION("const last()")
    {
        const shm::buffer<int>& cbuf = buf;
        CHECK(cbuf.last(1)[0] == 5);
    }
}

TEST_CASE("subspan()", "[buffer][spans]")
{
    shm::buffer<int> buf{10, 20, 30, 40, 50};

    SECTION("subspan(offset) from offset to end")
    {
        auto s = buf.subspan(2);
        REQUIRE(s.size() == 3);
        CHECK(s[0] == 30);
        CHECK(s[2] == 50);
    }

    SECTION("subspan(offset, count)")
    {
        auto s = buf.subspan(1, 3);
        REQUIRE(s.size() == 3);
        CHECK(s[0] == 20);
        CHECK(s[2] == 40);
    }

    SECTION("subspan(0) is full span")
    {
        auto s = buf.subspan(0);
        CHECK(s.size() == buf.size());
    }

    SECTION("subspan(size(), 0) is empty")
    {
        auto s = buf.subspan(buf.size(), 0);
        CHECK(s.empty());
    }

    SECTION("const subspan overloads")
    {
        const shm::buffer<int>& cbuf = buf;
        CHECK(cbuf.subspan(0, 2)[1] == 20);
        CHECK(cbuf.subspan(4)[0]    == 50);
    }
}

TEST_CASE("iterators", "[buffer][iterators]")
{
    /*
        Test Cases:

        - range-for produces correct values
        - iterator check via std::accumulate
        - begin == end for empty buffer
        - const iterator check
        - const iterator comparison
        - reverse iterator check
        - const reverse iterator check
    */

    shm::buffer<int> buf{1, 2, 3, 4, 5};

    SECTION("range-for produces correct values")
    {
        int expected = 1;
        for (auto v : buf) {
            CHECK(v == expected++);
        }
    }

    SECTION("iterator check via std::accumulate")
    {
        int sum = std::accumulate(buf.begin(), buf.end(), 0);
        CHECK(sum == 15);
    }

    SECTION("begin == end for empty buffer")
    {
        shm::buffer<int> empty;
        CHECK(empty.begin() == empty.end());
    }

    SECTION("const iterator check")
    {
        const shm::buffer<int>& cbuf = buf;
        int expected = 1;
        for (auto it = cbuf.begin(); it != cbuf.end(); ++it) {
            CHECK(*it == expected++);
        }
    }

    SECTION("const iterator comparison")
    {
        CHECK(buf.cbegin() == buf.begin());
        CHECK(buf.cend()   == buf.end());
    }
    SECTION("reverse iterator check")
    {
        std::vector<int> reversed(buf.rbegin(), buf.rend());
        CHECK(reversed == std::vector<int>{5, 4, 3, 2, 1});
    }

    SECTION("const reverse iterator check")
    {
        std::vector<int> reversed(buf.crbegin(), buf.crend());
        CHECK(reversed == std::vector<int>{5, 4, 3, 2, 1});

        const shm::buffer<int>& cbuf = buf;
        std::vector<int> reversed2(cbuf.rbegin(), cbuf.rend());
        CHECK(reversed2 == std::vector<int>{5, 4, 3, 2, 1});
    }

}

TEST_CASE("fill()", "[buffer][modifiers]")
{
    /*
        Test Cases:

        - Fills all elements
        - Fill on empty buffer is a no-op
    */

    SECTION("Fills all elements")
    {
        shm::buffer<int> buf(5, 0);
        buf.fill(7);
        for (auto v : buf) {
            CHECK(v == 7);
        }
    }

    SECTION("Fill on empty buffer is a no-op")
    {
        shm::buffer<int> buf;
        REQUIRE_NOTHROW(buf.fill(1));
    }
}


TEST_CASE("operator== / operator!=", "[buffer][comparison]")
{
    /*
        Test Cases:

        - Equal buffers
        - Different values
        - Different sizes
        - Two empty buffers are equal
        - Empty vs non-empty
        - Self equality
        - Struct type equality
    */

    SECTION("Equal buffers")
    {
        shm::buffer<int> a{1, 2, 3};
        shm::buffer<int> b{1, 2, 3};
        CHECK(a == b);
        CHECK_FALSE(a != b);
    }

    SECTION("Different values")
    {
        shm::buffer<int> a{1, 2, 3};
        shm::buffer<int> b{1, 2, 4};
        CHECK(a != b);
        CHECK_FALSE(a == b);
    }

    SECTION("Different sizes")
    {
        shm::buffer<int> a{1, 2, 3};
        shm::buffer<int> b{1, 2};
        CHECK(a != b);
    }

    SECTION("Two empty buffers are equal")
    {
        shm::buffer<int> a;
        shm::buffer<int> b;
        CHECK(a == b);
    }

    SECTION("Empty vs non-empty")
    {
        shm::buffer<int> a;
        shm::buffer<int> b{1};
        CHECK(a != b);
    }

    SECTION("Self equality")
    {
        shm::buffer<int> a{1, 2, 3};
        CHECK(a == a);
    }

    SECTION("Struct type equality")
    {
        shm::buffer<TrivialPod> a{{TrivialPod{1, 1.f}, {2, 2.f}}};
        shm::buffer<TrivialPod> b{{TrivialPod{1, 1.f}, {2, 2.f}}};
        CHECK(a == b);
    }
}
