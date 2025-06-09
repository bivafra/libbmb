/**
 * Vector testing
 * @authors bivafra
 */

#include "containers/vector.h"

#include <exception>
#include <initializer_list>
#include <stdexcept>
#include <vector>

#include "common/types.h"
#include "gtest/gtest.h"
#include "utils/iterators.h"
#include "utils/move.h"
#include "utils/type_traits.h"

using bmb::Vector;

// Lists for vectors initialization.
// All tests are independent on the lists content.
std::initializer_list<int>               list_int      = {0, 1, 2, 3, 4};
std::initializer_list<ManagesHeapMemory> list_heap_mem = {{}, {}, {}, {}, {}};

TEST(VectorConstruction, InitiallyEmpty) {
    Vector<int> a;

    ASSERT_EQ(a.size(), 0) << "Default construted Vector is not empty";
    ASSERT_EQ(a.capacity(), 0) << "Default construted Vector is not empty";
    ASSERT_TRUE(a.isEmpty()) << "Default construted Vector is not empty";

    Vector<NoDefaultCtor> b;
    ASSERT_EQ(b.size(), 0) << "Default construted Vector is not empty";
    ASSERT_EQ(b.capacity(), 0) << "Default construted Vector is not empty";
    ASSERT_TRUE(b.isEmpty()) << "Default construted Vector is not empty";
}

TEST(VectorConstruction, InitializerList) {
    Vector<int> a(list_int);

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), list_int.begin()))
        << "Vector initialized from initializer_list is not equal to it";

    ASSERT_EQ(a.size(), list_int.size());
    // WARNING: weak test, relies on internal implementation that prones to change
    ASSERT_EQ(a.capacity(), list_int.size());

    // Same for class with heap allocations.
    Vector<ManagesHeapMemory> b(list_heap_mem);

    ASSERT_TRUE(bmb::equal(b.begin(), b.end(), list_heap_mem.begin()))
        << "Vector initialized from initializer_list is not equal to it";

    ASSERT_EQ(b.size(), list_heap_mem.size());
    ASSERT_EQ(b.capacity(), list_heap_mem.size());
}

TEST(VectorConstruction, OtherRange) {
    std::vector stl_a(list_int);
    Vector<int> a(stl_a.begin(), stl_a.end());

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), stl_a.begin()))
        << "Vector copied from range is not equal to it";

    // Same for class with heap allocations.
    std::vector               stl_b(list_heap_mem);
    Vector<ManagesHeapMemory> b(stl_b.begin(), stl_b.end());

    ASSERT_TRUE(bmb::equal(b.begin(), b.end(), stl_b.begin()))
        << "Vector copied from range is not equal to it";

    // Range from same type
    Vector<int> origin(list_int);
    Vector<int> copy(origin.begin(), origin.end());

    ASSERT_TRUE(bmb::equal(origin.begin(), origin.end(), copy.begin()))
        << "Vector copied from range is not equal to it";
}

TEST(VectorConstruction, CopyConstructor) {
    Vector<int> a(list_int);
    Vector<int> b(a);

    ASSERT_EQ(a.size(), b.size());
    ASSERT_EQ(a.capacity(), b.capacity());

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), b.begin()));

    // Same for class with heap allocations.
    Vector<ManagesHeapMemory> mem_a(list_heap_mem);
    Vector<ManagesHeapMemory> mem_b(mem_a);

    ASSERT_EQ(mem_a.size(), mem_b.size());
    ASSERT_EQ(mem_a.capacity(), mem_b.capacity());

    ASSERT_TRUE(bmb::equal(mem_a.begin(), mem_a.end(), mem_b.begin()));
}

TEST(VectorConstruction, MoveConstructor) {
    Vector<int> a(list_int);
    Vector<int> b(bmb::move(a));

    ASSERT_EQ(a.getRawData(), nullptr);

    ASSERT_EQ(a.size(), 0);
    ASSERT_EQ(b.size(), list_int.size());

    ASSERT_EQ(a.capacity(), 0);
    ASSERT_EQ(b.capacity(), list_int.size());

    ASSERT_TRUE(bmb::equal(b.begin(), b.end(), list_int.begin()));

    // Same for class with heap allocations.
    Vector<ManagesHeapMemory> mem_a(list_heap_mem);
    Vector<ManagesHeapMemory> mem_b(bmb::move(mem_a));

    ASSERT_EQ(mem_a.getRawData(), nullptr);

    ASSERT_EQ(mem_a.size(), 0);
    ASSERT_EQ(mem_b.size(), list_heap_mem.size());

    ASSERT_EQ(mem_a.capacity(), 0);
    ASSERT_EQ(mem_b.capacity(), list_heap_mem.size());

    ASSERT_TRUE(bmb::equal(mem_b.begin(), mem_b.end(), list_heap_mem.begin()));
}

TEST(VectorConstruction, CopyAssignment) {
    Vector<int> a;
    Vector<int> b(list_int);

    a = b;

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), b.begin()));

    // Same for class with heap allocations.
    Vector<ManagesHeapMemory> mem_a;
    Vector<ManagesHeapMemory> mem_b(list_heap_mem);

    mem_a = mem_b;
    ASSERT_TRUE(bmb::equal(mem_a.begin(), mem_a.end(), mem_b.begin()));
}

TEST(VectorConstruction, MoveAssignment) {
    Vector<int> a;
    Vector<int> b(list_int);

    a = bmb::move(b);

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), list_int.begin()));
    ASSERT_EQ(b.size(), 0);

    // Same for class with heap allocations.
    Vector<ManagesHeapMemory> mem_a;
    Vector<ManagesHeapMemory> mem_b(list_heap_mem);

    mem_a = bmb::move(mem_b);
    ASSERT_TRUE(bmb::equal(mem_a.begin(), mem_a.end(), list_heap_mem.begin()));
    ASSERT_EQ(mem_b.size(), 0);
}

TEST(VectorConstruction, N_Default_Elements) {
    Vector<int> a(5);

    ASSERT_EQ(a.size(), 5);
    // WARNING: weak test, capacity policy is prone to change
    ASSERT_EQ(a.capacity(), 5);
    for (int x : a) ASSERT_EQ(x, 0);

    Vector<ManagesHeapMemory> mem_a(5);

    // Same for class with heap allocations.
    ASSERT_EQ(mem_a.size(), 5);
    ASSERT_EQ(mem_a.capacity(), 5);
    for (auto x : mem_a) ASSERT_EQ(x, ManagesHeapMemory());
}

TEST(VectorConstruction, N_NonDefault_Elements) {
    Vector<int> a(5, 777);

    ASSERT_EQ(a.size(), 5);
    // WARNING: weak test, capacity policy is prone to change
    ASSERT_EQ(a.capacity(), 5);
    for (int x : a) ASSERT_EQ(x, 777);

    Vector<ManagesHeapMemory> mem_a(5, ManagesHeapMemory(777));

    // Same for class with heap allocations.
    ASSERT_EQ(mem_a.size(), 5);
    ASSERT_EQ(mem_a.capacity(), 5);
    for (auto x : mem_a) ASSERT_EQ(x, ManagesHeapMemory(777));
}

TEST(VectorConstruction, CTAD) {
    // N elementes with given value
    static_assert(bmb::is_same_v<
                      decltype(Vector(0, 0)),
                      Vector<int>>,
                  "Wrong deduced typed");

    // Initializer list
    static_assert(bmb::is_same_v<
                      decltype(Vector(list_int.begin(), list_int.end())),
                      Vector<int>>,
                  "Wrong deduced typed");

    // Other range
    std::vector<char> range;
    static_assert(bmb::is_same_v<
                      decltype(Vector(range.begin(), range.end())),
                      Vector<char>>,
                  "Wrong deduced typed");
}

TEST(VectorComparing, General) {
    Vector a = list_int;
    Vector b = list_int;

    ASSERT_TRUE(a == b);

    b.pushBack(5);
    ASSERT_TRUE(a != b);

    // Different length
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(a <= b);
    ASSERT_FALSE(b < a);
    ASSERT_FALSE(b <= a);

    // Same length, but 'a' is greater
    a.pushBack(6);
    ASSERT_TRUE(a > b);
    ASSERT_TRUE(a >= b);

    // 'b' is longer but 'a' is greater
    b.pushBack(10);
    ASSERT_TRUE(a > b);
    ASSERT_TRUE(a >= b);

    // Check C++20 comparasion rewriting
    ASSERT_TRUE(a != list_int);
    ASSERT_TRUE(a > list_int);
    ASSERT_TRUE(a >= list_int);
    ASSERT_TRUE(list_int < a);
    ASSERT_TRUE(list_int <= a);
};

TEST(VectorModification, EmplaceBackPicky) {
    auto check = [](auto&& cont) {
        cont.pushBack(1);
        ASSERT_EQ(cont[0], 1);
        ASSERT_EQ(cont.size(), 1);
        // WARNING: vector doesn't give guarantee about capacity.
        // Probably, it's better to use '>= 1'
        ASSERT_EQ(cont.capacity(), 1);

        cont.pushBack(2);
        ASSERT_EQ(cont[0], 1);
        ASSERT_EQ(cont[1], 2);
        ASSERT_EQ(cont.size(), 2);
        ASSERT_EQ(cont.capacity(), 2);

        cont.pushBack(3);
        ASSERT_EQ(cont[0], 1);
        ASSERT_EQ(cont[1], 2);
        ASSERT_EQ(cont[2], 3);
        ASSERT_EQ(cont.size(), 3);
        ASSERT_EQ(cont.capacity(), 4);

        cont.pushBack(4);
        ASSERT_EQ(cont[0], 1);
        ASSERT_EQ(cont[1], 2);
        ASSERT_EQ(cont[2], 3);
        ASSERT_EQ(cont[3], 4);
        ASSERT_EQ(cont.size(), 4);
        ASSERT_EQ(cont.capacity(), 4);

        cont.pushBack(5);
        ASSERT_EQ(cont[0], 1);
        ASSERT_EQ(cont[1], 2);
        ASSERT_EQ(cont[2], 3);
        ASSERT_EQ(cont[3], 4);
        ASSERT_EQ(cont[4], 5);
        ASSERT_EQ(cont.size(), 5);
        ASSERT_EQ(cont.capacity(), 8);
    };

    Vector<int> a;
    check(a);

    Vector<ManagesHeapMemory> b;
    // ManagesHeapMemory have implicit c-tor from int.
    // So no problem for reusing the lambda.
    check(b);
}

TEST(VectorModification, EmplaceBackMany) {
    Vector<int>      arr(100, 9);
    std::vector<int> stl_arr(100, 9);

    Vector<ManagesHeapMemory>      mem_arr(100);
    std::vector<ManagesHeapMemory> mem_stl_arr(100);

    size_t num_of_elems = 30'000;
    for (size_t i = 0; i < num_of_elems; ++i) {
        arr.emplaceBack(i);
        stl_arr.emplace_back(i);

        mem_arr.emplaceBack(i);
        mem_stl_arr.emplace_back(i);
    }

    ASSERT_TRUE(bmb::equal(arr.begin(), arr.end(), stl_arr.begin()));

    ASSERT_TRUE(bmb::equal(mem_arr.begin(), mem_arr.end(), mem_stl_arr.begin()));
}

TEST(VectorModification, PopBack) {
    Vector arr = list_int;

    arr.popBack();
    ASSERT_EQ(arr.size(), list_int.size() - 1);
    ASSERT_EQ(arr.back(), *bmb::prev(list_int.end(), 2));

    while (arr.size() > 0) arr.popBack();
}

TEST(VectorModification, Clear) {
    Vector a = list_int;
    a.clear();

    ASSERT_EQ(a.getRawData(), nullptr);
    ASSERT_EQ(a.size(), 0);
    ASSERT_EQ(a.capacity(), 0);

    // Check whether vector is usable after clear. Drawback - depends on copy assignment.
    a = list_int;
    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), list_int.begin()));
}

TEST(VectorModification, Reserve) {
    Vector<int> a;
    a.reserve(100);

    ASSERT_EQ(a.size(), 0);
    ASSERT_EQ(a.capacity(), 100);

    // Capacity should not change in pushBack
    a.pushBack(1);
    ASSERT_EQ(a.size(), 1);
    ASSERT_EQ(a.capacity(), 100);
}

TEST(VectorModification, Swap) {
    Vector<int> a = {1, 2, 3};
    Vector<int> b = {4, 5, 6};

    auto a_copy = a;
    auto b_copy = b;

    swap(a, b);

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), b_copy.begin()));

    ASSERT_TRUE(bmb::equal(b.begin(), b.end(), a_copy.begin()));
}

TEST(VectorExceptionSafety, At) {
    Vector a = list_int;

    ASSERT_EQ(a.at(0), *list_int.begin());

    ASSERT_THROW(a.at(list_int.size()), std::out_of_range);
}

// The test is not possible under valgrind - it doesn't allow
// new/new[] to throw.
// TEST(VectorExceptionSafety, RequireMuchMemory) {
//     Vector a = list_int;
//
//     try {
//         a.reserve(1'000'000'000);
//         FAIL() << "Vector should throw when required "
//                   "enomorous amount of mememory";
//     } catch (...) {}
//
//     ASSERT_EQ(bmb::equal(a.begin(), a.end(), list_int.begin()),
//               true);
// }

TEST(VectorExceptionSafety, ThrowInRealloc) {
    using ThrowType = ThrowOnNthChainCopy;

    // 1 copy for initial construction.
    // And throw on the next realloc.
    Vector<ThrowType> a(10, ThrowType(2, 777));

    for (auto& x : a) ASSERT_EQ(x, ThrowType(1, 777));

    ASSERT_THROW(a.pushBack(ThrowType(5, 777)), std::exception);

    for (auto& x : a) ASSERT_EQ(x, ThrowType(1, 777));
}

TEST(VectorExceptionSafety, ThrowInReallocPicky) {
    using ThrowType = ThrowOnNthChainCopy;

    // Throw on the next realloc but in the middle of the array.
    Vector<ThrowType> a;
    a.reserve(5);

    // Preserve 2 copies for pushing.
    ThrowType wont_throw(4, 777);
    ThrowType will_throw(3, 777);

    a.pushBack(wont_throw);
    a.pushBack(wont_throw);

    a.pushBack(will_throw);

    a.pushBack(wont_throw);
    a.pushBack(wont_throw);

    ASSERT_THROW(a.pushBack(ThrowType(2, 777)), std::exception);

    for (auto& x : a) ASSERT_EQ(x, ThrowType(1, 777));

    ASSERT_EQ(a.size(), 5);
    ASSERT_EQ(a.capacity(), 5);
}

TEST(VectorExceptionSafety, ThrowInPushLastElement) {
    using ThrowType = ThrowOnNthChainCopy;

    // Throw in construction of the new element
    // while reallocating.
    Vector<ThrowType> a;
    a.reserve(3);

    // Preserve 2 copies for pushing.
    ThrowType wont_throw(4, 777);

    a.pushBack(wont_throw);
    a.pushBack(wont_throw);
    a.pushBack(wont_throw);

    ASSERT_THROW(a.pushBack(ThrowType(1, 777)), std::exception);

    for (auto& x : a) ASSERT_EQ(x, ThrowType(1, 777));

    ASSERT_EQ(a.size(), 3);
    ASSERT_EQ(a.capacity(), 3);
}

TEST(VectorExceptionSafety, ThrowInConstructor) {
    // The test essentially allows tools like 'valgrind' check memory leakages
    using ThrowType = ThrowOnNthChainCopy;

    // Throw in the 3-rd object
    ThrowType objects[5] = {ThrowType(4, 777),
                            ThrowType(4, 777),

                            ThrowType(1, 777),  // will throw

                            ThrowType(4, 777),
                            ThrowType(4, 777)};

    // Construct from range
    ASSERT_THROW(Vector(objects, objects + 5), std::exception);

    // N same elements.
    ASSERT_THROW(Vector(5, ThrowType(1, 777)), std::exception);

    // Copy constructor
    Vector a(5, ThrowType(2, 777));

    ASSERT_THROW((Vector(a)), std::exception);
}
