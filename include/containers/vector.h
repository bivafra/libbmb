#pragma once
/**
 * @file vector.h
 * Implementation of vector
 * @authors bivafra
 */

#include <cstddef>
#include <initializer_list>
#include <stdexcept>

#include "algo/algo_base.h"
#include "details/vector_iterator.h"
#include "utils/allocator.h"
#include "utils/iterators.h"
#include "utils/move.h"
#include "utils/type_traits.h"

namespace bmb {

/**
 * @class Vector
 * @brief AllocatorAware dynamic array structure
 *
 * All methods provides strong/conditional exception safety.
 *
 * Notes:
 *      Memory incrementation coefficient is hardcoded and equals 2.
 *      Probably, clients will be able to change it dynamically in future.
 *
 *      Current state notes: no support of `insert` in arbitrary place of the array.
 *      User provided type must not delete move constructor.
 */
template <typename T, typename Allocator = PrimitiveAllocator>
// for now block const types. see c++ std::launder and raw operator delete call on const type.
    requires is_same_v<T, remove_const_t<T>>
class Vector {
    // TODO: add template constraints
    // TODO: add 'insert' support
    // TODO: add reverse_iterator

    using AllocTraits = AllocatorTraits<Allocator>;

    // Used to mark whether to construct new element.
    enum class PushDecide : char {
        kDoPush,
        kNoPush
    };

public:
    using value_type      = T;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using size_type       = size_t;
    using allocator_type  = Allocator;
    using iterator        = detail::BaseVectorIter<T, false>;
    using const_iterator  = detail::BaseVectorIter<T, true>;

    Vector() noexcept(noexcept(Allocator())) {}

    explicit Vector(const Allocator& alloc)
        : alloc_(alloc) {}

    /**
     * @brief Constructs `count` elements with given value.
     *
     * Allocates memory for exactly `count` elements. Then copies
     * given value into `count` elements.
     *
     * @param count Number of elements to construct
     * @param value Value to initialize the array. Has default value
     * @param alloc Container's allocator. Has default value
     *
     * @throws Provides strong exception guarantee. If exception is thrown
     * during coping the value - all constructed elements are destroyed,
     * allocated memory is deallocated and the exception is rethrown.
     */
    explicit Vector(size_t count, const value_type& value = value_type(),
                    const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        // Possible problem: there is allocation and
        // construction of the same number of elements, thus,
        // in next push there will be reallocation.
        // Possible solution: reserve more memory(e.g. 2*count)

        reserve(count);

        try {
            // Use size_ field for proper clear() usage in case of exception
            for (; size_ < capacity_; ++size_) {
                AllocTraits::construct(alloc_, arr_ + size_, value);
            }
        } catch (...) {
            // Don't forget destroy all constructed elements
            clear();
            throw;
        }
    }

    /**
     * @brief Initialize Vector with the given range.
     *
     * Constructs Vector with elements equal to the ones in `[first, last)`.
     *
     * @param first Start of the range
     * @param last End of the range
     * @param alloc Container's allocator. Has default value
     *
     * @throws Provides strong exception guarantee. If exception is thrown
     * during coping the range's value - all constructed elements are destroyed,
     * allocated memory is deallocated and the exception is rethrown.
     */
    template <InputIterator Iter>
    Vector(Iter first, Iter last,
           const Allocator& alloc = Allocator())
        : alloc_(alloc) {
        rangeInit(first, last);
    }

    /**
     * @brief Initialize Vector with the given initializer_list.
     *
     * Constructs Vector with elements equal to the ones in the initializer_list.
     *
     * @param init_list Initializer list to copy
     * @param alloc Container's allocator. Has default value
     *
     * @throws Provides strong exception guarantee. If exception is thrown
     * during coping the initializer_list's value - all constructed elements are destroyed,
     * allocated memory is deallocated and the exception is rethrown.
     */
    Vector(std::initializer_list<value_type> init_list,
           const Allocator&                  alloc = Allocator())
        : alloc_(alloc) {
        rangeInit(init_list.begin(), init_list.end());
    }

    /**
     * @brief Copy constructor
     *
     * @throws Provides strong exception guarantee. If exception is thrown
     * during coping - all constructed elements are destroyed,
     * allocated memory is deallocated and the exception is rethrown.
     */
    Vector(const Vector& other)
        : alloc_(other.alloc_) {
        rangeInit(other.begin(), other.end());
    }

    Vector(Vector&& other) noexcept(noexcept(Allocator(move(other.alloc_))))
        : arr_(other.arr_)
        , size_(other.size_)
        , capacity_(other.capacity_)
        , alloc_(move(other.alloc_)) {
        // NOTE: Another possible implementation: default construct + swap(*this, other).
        // However it has a downside: Allocator must be default constructible,
        // which either inefficent or inconvenient.
        other.arr_      = nullptr;
        other.size_     = 0;
        other.capacity_ = 0;
    }

    /**
     * @brief Copy and Move assignment operator.
     *
     * Implemented through copy/move-and-swap idiom.
     *
     * @throws Strong safety when allocators swap throws.
     * Otherwise noexcept.
     */
    Vector& operator=(Vector other) {
        swap(*this, other);
        return *this;
    }

    ~Vector() { clear(); }

    /**
     * @brief Efficiently swaps data of the Vectors.
     *
     * All iterators and references remain valid.
     *
     * @param a Vector to swap
     * @param b Vector to swap
     *
     * @throws Strong safety when allocators swap throws.
     * Otherwise noexcept.
     */
    friend void swap(Vector& a, Vector& b) {
        // Firstly try to swap allocators to provide strong exception safety.
        swap(a.alloc_, b.alloc_);

        swap(a.arr_, b.arr_);
        swap(a.size_, b.size_);
        swap(a.capacity_, b.capacity_);
    }

    /**
     * @brief Checks whether Vector has elements.
     *
     * @return true - if Vector's size == 0. false otherwise.
     */
    bool isEmpty() const noexcept { return size_ == 0; }

    /**
     * @brief Preallocates 'new_capacity' memory.
     *
     * If `new_capacity` > the actual one - allocates
     * new memory, moves elements into it, deallocates
     * previous storage. All references and iterators are invalidated.
     *
     * If new capacity <= the actual one, the function has no effect.
     *
     * @param new_capacity Number of elements to preallocate memory for
     *
     * @throws Conditional safety. Strong safety, but the basic one in case:
     * if template type has a move c-tor and it throws during
     * moving. All elements in new storage will be destroyed,
     * thus, initial array will start with 'empty' elements and finish with initial ones.
     */
    void reserve(size_t new_capacity) {
        pushingRealloc(new_capacity, PushDecide::kNoPush);
    }

    /// Wrapper for `emplaceBack`. See docs for `emplaceBack`.
    void pushBack(value_type value) {
        emplaceBack(move(value));
    }

    /**
     * @brief Constructs new element from given `args`.
     *
     * If array has memory for one more element - constructs an element at
     * the end of the storage. Only end() iterator is invalidated.
     *
     * If array is full - reallocates storage with the larger capacity and
     * constructs an element there. All references and iterators are invalidated.
     *
     * After this operation Vector's size is incremented by 1.
     *
     * Time complexity: amortized O(1).
     *
     * @param args... Arguments to perfect forward in the element c-tor
     *
     * @return Reference to the inserted element
     *
     * @throws Conditional safety. Strong safety, but the basic one in case:
     * if template type has move c-tor and it throws during
     * moving. All elements in new storage will be destroyed,
     * thus, initial array will start with 'empty' elements and finish with initial ones.
     */
    template <typename... Args>
    reference emplaceBack(Args&&... args) {
        // There are several reasons why we must differentiate
        // realloc + push and simple push:
        //      1) Major reason: case v.emplaceBack(v[5]); for more
        //         details see `pushingRealloc` definition. Order of operations does matter.
        //      2) Minor reason: pointer to the raw array(obtained from v.getRawData())
        //         might point to the previous storage while we allocated new one.
        //      3) Minor reason: follwing from the standard definition of the
        //         strong exception guarantee.

        constexpr size_t grow_coeff = 2;
        constexpr size_t first_cap  = 1;
        if (size_ == capacity_) {
            size_t new_cap = capacity_ > 0 ? grow_coeff * capacity_
                                           : first_cap;

            pushingRealloc(new_cap, PushDecide::kDoPush,
                           forward<Args>(args)...);
        } else {
            AllocTraits::construct(alloc_, arr_ + size_,
                                   forward<Args>(args)...);
            ++size_;
        }

        return back();
    }

    /**
     * @brief Destroyes the last object in the array.
     *
     * The end() iterator is invalidated.
     * If array is empty - undefined behaviour.
     *
     * @throws noexcept
     */
    void popBack() noexcept {
        AllocTraits::destroy(alloc_, arr_ + size_ - 1);
        --size_;
    }

    /**
     * @brief Destroyes the last object in the array.
     *
     * The end() iterator is invalidated.
     *
     * @throws Throws std::logic_error if array is empty
     */
    void boundPopBack() {
        if (size_ == 0)
            throw std::logic_error(
                "Try to remove the last "
                "element of the empty Vector");
        popBack();
    }

    /**
     * @brief Empties the Vector.
     *
     * Destroyes all elements in the array and
     * deallocate storage. All fields are nullified.
     *
     * @throws noexcept
     */
    void clear() noexcept {
        for (size_t i = 0; i < size_; ++i) {
            AllocTraits::destroy(alloc_, arr_ + i);
        }
        AllocTraits::deallocate(alloc_, arr_, capacity_);

        arr_      = nullptr;
        size_     = 0;
        capacity_ = 0;
    }

    /**
     * Returns current number of elements in the Vector.
     */
    size_t size() const noexcept { return size_; }

    /**
     * Returns current Vector's capacity.
     */
    size_t capacity() const noexcept { return capacity_; }

    /**
     * Returns pointer to the raw underlying array.
     */
    pointer getRawData() noexcept { return arr_; }

    /**
     * Returns pointer to the raw underlying array.
     */
    const_pointer getRawData() const noexcept { return arr_; }

    /**
     * Returns element at n-th index in the array.
     * If index is out of bounds - undefined behaviour.
     */
    reference operator[](size_t n) noexcept { return arr_[n]; }

    /**
     * Returns element at n-th index in the array.
     * If index is out of bounds - undefined behaviour.
     */
    const_reference operator[](size_t n) const noexcept { return arr_[n]; }

    /**
     * Returns element at n-th index in the array.
     * If index is out of bounds, throws std::out_of_range
     */
    reference at(size_t n) {
        rangeCheck(n);
        return arr_[n];
    }

    /**
     * Returns element at n-th index in the array.
     * If index is out of bounds, throws std::out_of_range
     */
    const_reference at(size_t n) const {
        rangeCheck(n);
        return arr_[n];
    }

    /**
     * Returns first element in the array.
     * If index is out of bounds - undefined behaviour.
     */
    reference front() noexcept { return arr_[0]; }

    /**
     * Returns first element in the array.
     * If index is out of bounds - undefined behaviour.
     */
    const_reference front() const noexcept { return arr_[0]; }

    /**
     * Returns last element in the array.
     * If index is out of bounds - undefined behaviour.
     */
    reference back() noexcept { return arr_[size_ - 1]; }

    /**
     * Returns last element in the array.
     * If index is out of bounds - undefined behaviour.
     */
    const_reference back() const noexcept { return arr_[size_ - 1]; }

    // Iterators

    iterator begin() noexcept { return iterator(arr_); }
    iterator end() noexcept { return iterator(arr_ + size_); }

    const_iterator begin() const noexcept { return const_iterator(arr_); }
    const_iterator end() const noexcept { return const_iterator(arr_ + size_); }

    const_iterator cbegin() const noexcept { return const_iterator(arr_); }
    const_iterator cend() const noexcept { return const_iterator(arr_ + size_); }

    // reverse_iterator rbegin() ....

    bool operator==(const Vector& other) const {
        return size() == other.size()
               && equal(begin(), end(), other.begin());
    }

    auto operator<=>(const Vector& other) const {
        return lexicographical_compare_three_way(begin(), end(),
                                                 other.begin(), other.end());
    }

private:
    /**
     * @brief Checks whether index is out of bounds.
     *
     * @throws Throws std::out_of_range if the index is out of bounds.
     */
    void rangeCheck(size_t index) const {
        if (index >= size_) throw std::out_of_range("Vector index out of range.");
    }

    /**
     * @brief Initialize the Vector with elements from the given range.
     *
     * Vector's size and capacity will be equal to (last - first).
     *
     * Time complexity: O(last - first)
     *
     * If given iterators are at least forward - make 1 allocation
     * of (last - first) elements and constructs them.
     * If iterators are input - performs (last - first) emplaceBack calls.
     *
     * @param first Start of the range
     * @param last End of the range
     *
     * @throws Provides strong exception guarantee. If exception is thrown
     * during coping the range's value - all constructed elements are destroyed,
     * allocated memory is deallocated and the exception is rethrown.
     */
    template <InputIterator Iter>
    void rangeInit(Iter first, Iter last) {
        using iter_category = IteratorTraits<Iter>::iterator_category;

        // If able to find the length of the range - do it even for O(last - first),
        // because otherwise we must emplaceBack one by one doing many memory
        // reallocations.
        if constexpr (is_base_of_v<forward_iter_tag, iter_category>) {
            rangeInitFastImpl(first, last);
        } else rangeInitSlowImpl(first, last);
    }

    /// See `rangeInit`
    template <InputIterator Iter>
    void rangeInitFastImpl(Iter first, Iter last) {
        // The key point here: we use capability of the forward iterator to be
        // readed/incremented multiple times.
        size_t n = distance(first, last);
        reserve(n);

        try {
            // Use size_ field for proper clear() in case of exception
            for (; size_ < capacity_; ++size_, ++first) {
                AllocTraits::construct(alloc_, arr_ + size_, *first);
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    /// See `rangeInit`
    template <InputIterator Iter>
    void rangeInitSlowImpl(Iter first, Iter last) {
        try {
            for (; first != last; ++first) emplaceBack(*first);
        } catch (...) {
            // If exception in copy c-tor - clear all constructed elements
            clear();
            throw;
        }
    }

    /**
     * @brief Moves all array elements into the new larger storage,
     * optionally, constructs a new element from `args`.
     *
     * Allocates an array for new_capacity elements. If
     * do_push tells to construct new element - constructs it
     * in the new array. Then moves elements from the old array
     * to the new one. After that, deallocates previous array.
     *
     * If new_capacity <= the actual one - the function has no effect.
     *
     * @param new_capacity Size of the new allocated storage
     * @param do_push Whether to construct new element from given args
     * @param args... Arguments to pass in the new element constructor.
     *
     * @throws Conditional safety. Strong safety, but the basic one in case:
     * if template type has a move c-tor and it throws during
     * moving. All elements in new storage will be destroyed,
     * thus, initial array will start with 'empty' elements and finish with initial ones.
     */
    template <typename... Args>
    void pushingRealloc(size_t new_capacity, PushDecide do_push,
                        Args&&... args) {
        // NOTE: Order of operations does matter.
        // Consider case v.emplaceBack(v[5]). If we firstly
        // move all elements, then construct new value at the end -
        // we miss this v[5] reference, since it could be moved. Thefore, we
        // firstly must construct new value and only then move others.
        //
        // NOTE: We don't use move_if_noexcept, instead we always move.

        if (new_capacity <= capacity_) return;

        T* new_arr = nullptr;

        // Need for catch clause to destroy already constructed elements
        size_t index = 0;

        // Shows whether a new element was constructed
        bool was_new_pushed = false;

        new_arr = AllocTraits::template allocate<value_type>(alloc_, new_capacity);

        try {
            // Firstly push a new element
            if (do_push == PushDecide::kDoPush) {
                AllocTraits::construct(alloc_, new_arr + size_,
                                       forward<Args>(args)...);
                was_new_pushed = true;
            }

            // Then move all existing ones
            for (; index < size_; ++index) {
                // WARNING: If the template type has excplicitly
                // delete move c-tor, there's CE. Compile time check
                // through is_move_constructible concept will fix this. But
                // the same requires in the several places in the code. So
                // for now stay with this behaviour.
                AllocTraits::construct(alloc_, new_arr + index,
                                       move(arr_[index]));
            }

        } catch (...) {
            // Exception may occur either while pushing new element
            // or while coping/moving old elements. In the 1-st case do nothing.
            // In the 2-nd case destroy copied/moved elements.

            if (was_new_pushed) {
                AllocTraits::destroy(alloc_, new_arr + size_);
            }

            for (size_t old_index = 0; old_index < index; ++old_index) {
                AllocTraits::destroy(alloc_, new_arr + old_index);
            }
            AllocTraits::deallocate(alloc_, new_arr, new_capacity);

            throw;
        }

        // In success destroy and deallocate previous storage.
        // Don't forget save new size, since clear will nullify all fields.
        size_t new_size = size_ + (was_new_pushed ? 1 : 0);

        clear();

        arr_      = new_arr;
        size_     = new_size;
        capacity_ = new_capacity;
    }

    pointer arr_      = nullptr;
    size_t  size_     = 0;
    size_t  capacity_ = 0;

    [[no_unique_address]]
    Allocator alloc_;
};

// Explicit template deduction guide
template <InputIterator Iter, typename Allocator = PrimitiveAllocator>
Vector(Iter, Iter, Allocator = Allocator()) -> Vector<typename IteratorTraits<Iter>::value_type, Allocator>;

}  // namespace bmb
