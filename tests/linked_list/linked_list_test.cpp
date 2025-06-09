/**
 * Singly Linked list testing
 * @authors bivafra
 */

#include "containers/linked_list.h"

#include <cstddef>
#include <exception>

#include "algo/algo_base.h"
#include "common/types.h"
#include "containers/vector.h"
#include "gtest/gtest.h"
#include "utils/type_traits.h"

// NOTE: tests mostly use `FatList`. `TinyList` is also tested
// where it is reasonable.

using FatList  = bmb::LListFat<ManagesHeapMemory>;
using TinyList = bmb::LListTiny<ManagesHeapMemory>;

// List for LinkedList initialization.
// All tests are independent on the list content.
static std::initializer_list<ManagesHeapMemory> init_list = {{0}, {1}, {2}, {3}, {4}};

TEST(LListConstruction, InitiallyEmpty) {
    FatList a;

    ASSERT_EQ(a.size(), 0);
    ASSERT_TRUE(a.isEmpty());

    // Same for tiny

    TinyList b;

    ASSERT_EQ(b.size(), 0);
    ASSERT_TRUE(b.isEmpty());
}

TEST(LListConstruction, NoDefaultConstructor) {
    bmb::LListFat<NoDefaultCtor> a;
    a.emplaceBack(1);
    a.insertAfter(a.begin(), 1);
    a.insertAfter(a.begin(), 1);

    a.popBack();
    a.popFront();
    a.eraseAfter(a.beforeBegin());

    // Must be empty
    ASSERT_EQ(a, decltype(a)());
}

TEST(LListConstruction, InitializerList) {
    FatList a(init_list);

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), init_list.begin()));

    ASSERT_EQ(a.size(), init_list.size());

    // Same for tiny

    TinyList b(init_list);

    ASSERT_TRUE(bmb::equal(b.begin(), b.end(), init_list.begin()));

    ASSERT_EQ(b.size(), init_list.size());
}

TEST(LListConstruction, OtherRange) {
    bmb::Vector stl_a(init_list);
    FatList     a(stl_a.begin(), stl_a.end());

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), stl_a.begin()));

    ASSERT_EQ(a.size(), stl_a.size());
    ASSERT_EQ(a.back(), stl_a.back());
}

TEST(LListConstruction, CopyConstructor) {
    FatList a(init_list);
    FatList b(a);

    ASSERT_EQ(a.size(), b.size());
    ASSERT_EQ(a.back(), b.back());
    ASSERT_EQ(a, b);

    // Same for tiny

    TinyList tiny_a(init_list);
    TinyList tiny_b(tiny_a);

    ASSERT_EQ(tiny_a.size(), tiny_b.size());
    ASSERT_EQ(tiny_a.back(), tiny_b.back());
    ASSERT_EQ(tiny_a, tiny_b);
}

TEST(LListConstruction, MoveConstructor) {
    FatList a(init_list);
    FatList b(bmb::move(a));

    ASSERT_EQ(a.getRawFirstNode(), nullptr);

    ASSERT_EQ(a.size(), 0);
    ASSERT_EQ(b.size(), init_list.size());

    ASSERT_EQ(b, init_list);
}

TEST(LListConstruction, CopyAssignment) {
    FatList a(init_list);
    FatList b;

    b = a;

    ASSERT_EQ(a, b);
    ASSERT_EQ(b, init_list);
    ASSERT_EQ(a.size(), b.size());
    ASSERT_EQ(a.back(), b.back());
}

TEST(LListConstruction, MoveAssignment) {
    FatList a(init_list);
    FatList b;

    b = bmb::move(a);

    ASSERT_EQ(b, init_list);
    ASSERT_EQ(a.size(), 0);
}

TEST(LListConstruction, N_DefaultElements) {
    FatList a(5);

    ASSERT_EQ(a.size(), 5);

    for (auto x : a) ASSERT_EQ(x, ManagesHeapMemory());

    ASSERT_EQ(a.back(), ManagesHeapMemory());
}

TEST(LListConstruction, N_NonDefaultElements) {
    FatList a(5, 111);

    ASSERT_EQ(a.size(), 5);

    for (auto x : a) ASSERT_EQ(x, 111);

    ASSERT_EQ(a.back(), 111);
}

TEST(LListConstruction, CTAD) {
    static_assert(bmb::is_same_v<decltype(bmb::LinkedList(init_list.begin(), init_list.end())),
                                 bmb::LinkedList<ManagesHeapMemory>>,
                  "Wrong deduced typed");
}

TEST(LListComparing, General) {
    FatList a = init_list;
    FatList b = init_list;

    ASSERT_TRUE(a == b);

    b.emplaceBack(5);

    ASSERT_TRUE(a != b);

    // Different length
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(a <= b);
    ASSERT_FALSE(b < a);
    ASSERT_FALSE(b <= a);

    // Same length, but `a` is greater
    a.emplaceBack(6);
    ASSERT_TRUE(a > b);
    ASSERT_TRUE(a >= b);
}

TEST(LListComparing, ClassSize) {
    // For TinyList store only 1 pointer
    static_assert(sizeof(bmb::LinkedList<int, false, false>) == 8,
                  "Wrong sizeof for TinyList");
    // pointer to head + length
    static_assert(sizeof(bmb::LinkedList<int, true, false>) == 16,
                  "Wrong sizeof for TrackSize");
    // pointer to head + pointer to last
    static_assert(sizeof(bmb::LinkedList<int, false, true>) == 16,
                  "Wrong sizeof for TrackLast");
    // pointer to head + pointer to last + length
    static_assert(sizeof(bmb::LinkedList<int, true, true>) == 24,
                  "Wrong sizeof for FatList");
}

TEST(LListModification, EmplaceBackPicky) {
    auto check = [](auto&& cont) {
        auto it = cont.beforeBegin();

        cont.emplaceBack(1);
        ++it;

        ASSERT_EQ(*it, 1);
        ASSERT_EQ(cont.size(), 1);

        cont.emplaceBack(2);
        ++it;
        ASSERT_EQ(*it, 2);
        ASSERT_EQ(cont.size(), 2);

        cont.emplaceBack(3);
        ++it;
        ASSERT_EQ(*it, 3);
        ASSERT_EQ(cont.size(), 3);

        it = cont.begin();
        ASSERT_EQ(*it, 1);
        ++it;
        ASSERT_EQ(*it, 2);
        ++it;
        ASSERT_EQ(*it, 3);
    };

    FatList a;
    check(a);

    TinyList b;
    check(b);
}

TEST(LListModification, EmplaceBackManyFastLastAccess) {
    FatList                        list;
    bmb::Vector<ManagesHeapMemory> arr;

    const size_t num_of_elems = 30'000;
    for (size_t i = 0; i < num_of_elems; ++i) {
        arr.emplaceBack(i);
        list.emplaceBack(i);
    }

    ASSERT_EQ(arr.size(), list.size());
    ASSERT_TRUE(bmb::equal(list.begin(), list.end(), arr.begin()));
}

TEST(LListModification, EmplaceBackManySlowLastAccess) {
    TinyList                       list;
    bmb::Vector<ManagesHeapMemory> arr;

    // PERF: really slow loop: `O(n^2)`.
    // 50'000 elements takes about 15 seconds,
    // while with fast last access it takes 0.3 seconds.
    const size_t num_of_elems = 5'000;
    for (size_t i = 0; i < num_of_elems; ++i) {
        arr.emplaceBack(i);
        list.emplaceBack(i);
    }

    ASSERT_EQ(arr.size(), list.size());
    ASSERT_TRUE(bmb::equal(list.begin(), list.end(), arr.begin()));
}

// Checks that `actual` is equal to `expected`, including sizes and last pointers
static bool checkListStructure(const FatList& actual, const FatList& expected) {
    if (actual != expected || actual.size() != expected.size()) {
        return false;
    }

    if (expected.isEmpty()) {
        // For empty lists last_ pointer must point to the head_
        return actual.beforeBegin() == actual.beforeEnd();
    }
    // For non-empty just check last elements
    return actual.back() == expected.back();
}

TEST(LListModification, EmplaceFront) {
    FatList a;

    auto before_begin_it = a.beforeBegin();

    a.emplaceFront(1);
    ASSERT_EQ(*bmb::next(before_begin_it), 1);

    a.emplaceFront(2);
    ASSERT_EQ(*bmb::next(before_begin_it), 2);

    a.emplaceFront(3);
    ASSERT_EQ(*bmb::next(before_begin_it), 3);

    ASSERT_TRUE(checkListStructure(a, {3, 2, 1}));
}

TEST(LListModification, EmplaceAfter) {
    FatList a;

    // 1
    a.emplaceAfter(a.beforeBegin(), 1);
    // 1 2
    a.emplaceAfter(a.begin(), 2);
    // 3 1 2
    a.emplaceAfter(a.beforeBegin(), 3);
    // 3 1 4 2
    a.emplaceAfter(bmb::next(a.begin()), 4);
    // 3 1 5 4 2
    a.emplaceAfter(bmb::next(a.begin()), 5);
    // 3 1 5 6 4 2
    a.emplaceAfter(bmb::next(a.begin(), 2), 6);
    // 3 1 5 6 4 2 7
    a.emplaceAfter(a.beforeEnd(), 7);

    ASSERT_TRUE(checkListStructure(a, {3, 1, 5, 6, 4, 2, 7}));
}

TEST(LListModification, InsertAfterRange) {
    FatList a;

    bmb::Vector<ManagesHeapMemory> start_arr = {1, 2, 3, 4};

    a.insertAfter(a.beforeBegin(), start_arr.begin(), start_arr.end());

    ASSERT_TRUE(bmb::equal(a.begin(), a.end(), start_arr.begin()));

    FatList to_add = {7, 7, 7, 7};

    // 1 2 3 7 7 7 7 4
    a.insertAfter(bmb::next(a.begin(), 2), to_add.begin(), to_add.end());

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 7, 7, 7, 7, 4}));
}

TEST(LListModification, EraseAfterSingle) {
    FatList a = {1, 2, 3, 4, 5};

    // 1 3 4 5
    a.eraseAfter(a.begin());
    // 1 3 5
    a.eraseAfter(bmb::next(a.begin()));
    // 3 5
    a.eraseAfter(a.beforeBegin());

    ASSERT_TRUE(checkListStructure(a, {3, 5}));
}

TEST(LListModification, EraseAfterRange) {
    FatList a = {1, 2, 3, 4, 5};

    // Make empty
    a.eraseAfter(a.beforeBegin(), a.end());

    ASSERT_TRUE(checkListStructure(a, {}));

    a = {1, 2, 3, 4, 5};

    // 1 5
    a.eraseAfter(a.begin(), a.beforeEnd());

    ASSERT_TRUE(checkListStructure(a, {1, 5}));
}

TEST(LListModification, PopFront) {
    FatList a = {1, 2, 3, 4, 5};

    // 2 3 4 5
    a.popFront();
    // 3 4 5
    a.popFront();
    // 4 5
    a.popFront();

    ASSERT_TRUE(checkListStructure(a, {4, 5}));
}

TEST(LListModification, PopBack) {
    FatList a = {1, 2, 3, 4, 5};

    // 1 2 3 4
    a.popBack();
    // 1 2 3
    a.popBack();
    // 1 2
    a.popBack();

    ASSERT_TRUE(checkListStructure(a, {1, 2}));

    // 1
    a.popBack();
    // []
    a.popBack();

    ASSERT_TRUE(checkListStructure(a, {}));
}

TEST(LListModification, MergeGeneral) {
    FatList a = {1, 3, 5};
    FatList b = {2, 4, 6};

    a.merge(b);

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 4, 5, 6}));
    ASSERT_TRUE(checkListStructure(b, {}));
}
TEST(LListModification, MergeEmpty) {
    FatList a = {};
    FatList b = {1, 2, 3};

    a.merge(b);

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3}));
    ASSERT_TRUE(checkListStructure(b, {}));

    // Try merge non empty `a` with empty `b` - should
    // not have effects
    a.merge(b);

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3}));
    ASSERT_TRUE(checkListStructure(b, {}));
}

TEST(LListModification, SpliceAfterGeneral) {
    FatList a = {1, 2, 6, 7};
    FatList b = {3, 4, 5};

    a.spliceAfter(bmb::next(a.begin()), b);

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 4, 5, 6, 7}));
}

TEST(LListModification, SpliceAfterHead) {
    FatList a = {4, 5, 6, 7};
    FatList b = {1, 2, 3};

    a.spliceAfter(a.beforeBegin(), b);

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 4, 5, 6, 7}));
}

TEST(LListModification, SpliceAfterLast) {
    FatList a = {1, 2, 3};
    FatList b = {4, 5, 6, 7};

    a.spliceAfter(a.beforeEnd(), b);

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 4, 5, 6, 7}));
}

TEST(LListModification, SpliceAfterEmpty) {
    FatList a = {1, 2, 3, 4, 5};
    FatList b;

    a.spliceAfter(a.beforeBegin(), b);
    a.spliceAfter(a.begin(), b);
    a.spliceAfter(bmb::next(a.begin()), b);
    a.spliceAfter(a.beforeEnd(), b);

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 4, 5}));
}

TEST(LListModification, SpliceAfterRange) {
    FatList a = {1, 2, 6, 7};
    FatList b = {1, 2, 3, 4, 5};

    // 1 2 3 4 5 6 7
    a.spliceAfter(bmb::next(a.begin()), b, bmb::next(b.begin()), b.end());
    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 4, 5, 6, 7}));
}

TEST(LListModification, SpliceAfterInSameList) {
    // Unsorted
    FatList a = {1, 2, 6, 7, 3, 4, 5, 8, 9};

    // Make sorted: 1 2 3 4 5 6 7 8 9
    a.spliceAfter(bmb::next(a.begin(), 6), a,
                  bmb::next(a.begin()), bmb::next(a.begin(), 4));

    ASSERT_TRUE(checkListStructure(a, {1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST(LListModification, Reverse) {
    FatList a = {1, 2, 3, 4, 5};
    a.reverse();
    a.reverse();
    a.reverse();
    ASSERT_TRUE(checkListStructure(a, {5, 4, 3, 2, 1}));
}

TEST(LListModification, Swap) {
    FatList a = {1, 2, 3};
    FatList b = {4, 5, 6};

    bmb::swap(a, b);

    ASSERT_TRUE(checkListStructure(a, {4, 5, 6}));
    ASSERT_TRUE(checkListStructure(b, {1, 2, 3}));
}

TEST(LListModification, Clear) {
    FatList a(init_list);

    a.clear();
    ASSERT_TRUE(checkListStructure(a, {}));
}

TEST(LListAccess, FastLastElem) {
    FatList a;

    const size_t num_of_elems = 30'000;
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.emplaceFront(i);
    }

    // WARN: compiler may optimize this code
    // and remove this loop entirely
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.back();
    }
}

TEST(LListAccess, SlowLastElem) {
    TinyList a;

    const size_t num_of_elems = 5'000;
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.emplaceFront(i);
    }

    // WARN: compiler may optimize this code
    // and remove this loop entirely
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.back();
    }
}

TEST(LListAccess, FastSize) {
    FatList a;

    const size_t num_of_elems = 30'000;
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.emplaceFront(i);
    }

    // WARN: compiler may optimize this code
    // and remove this loop entirely
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.size();
    }
}

TEST(LListAccess, SlowSize) {
    TinyList a;

    const size_t num_of_elems = 5'000;
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.emplaceFront(i);
    }

    // WARN: compiler may optimize this code
    // and remove this loop entirely
    for (size_t i = 0; i < num_of_elems; ++i) {
        a.size();
    }
}

using ThrowType    = ThrowOnNthChainCopy;
using ListForThrow = bmb::LListFat<ThrowType>;

TEST(LListExceptionSafety, Construction) {
    ThrowType objects[5] = {ThrowType(5),
                            ThrowType(5),
                            ThrowType(1),  // will throw
                            ThrowType(5),
                            ThrowType(5)};

    ASSERT_THROW(ListForThrow(objects, objects + 5), std::exception);

    // Copy c-tor
    ListForThrow a(5, ThrowType(2));

    ASSERT_THROW((ListForThrow(a)), std::exception);
}

TEST(LListExceptionSafety, InsertAfterRange) {
    ListForThrow a;
    for (int i = 1; i < 6; ++i) {
        a.emplaceBack(5);
    }

    ThrowType objects[5] = {ThrowType(5),
                            ThrowType(5),
                            ThrowType(1),  // will throw
                            ThrowType(5),
                            ThrowType(5)};

    ASSERT_THROW(a.insertAfter(a.begin(), objects, objects + 5), std::exception);
}

TEST(LListExceptionSafety, insertAfterLast) {
    ListForThrow a;
    for (int i = 1; i < 6; ++i) {
        a.emplaceBack(5);
    }

    ASSERT_THROW(a.insertAfter(a.beforeEnd(), ThrowType(1)), std::exception);
}

TEST(LListExceptionSafety, insertAfterHead) {
    ListForThrow a;
    for (int i = 1; i < 6; ++i) {
        a.emplaceBack(5);
    }

    ASSERT_THROW(a.insertAfter(a.beforeBegin(), ThrowType(1)), std::exception);
}
