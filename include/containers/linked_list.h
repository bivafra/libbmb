#pragma once
/**
 * @file linked_list.h
 * Singly linked list implementation
 * @authors bivafra
 */

#include <cstddef>
#include <initializer_list>
#include <stdexcept>

#include "algo/algo_base.h"
#include "details/llist_iterator.h"
#include "details/llist_nodes.h"
#include "utils/allocator.h"
#include "utils/compare.h"
#include "utils/iterators.h"
#include "utils/move.h"
#include "utils/type_traits.h"

namespace bmb {

/**
 * @class LinkedList
 * @brief Singly linked list.
 *
 * @tparam T type to hold
 * @tparam TrackSize enables fast size access
 * @tparam TrackLast enables fast last element access
 * @tparam Allocator allocator to use for memory management
 *
 * This implementation may be used in several modes:
 *      1) Memory saving:
 *          sizeof this class equals 1 pointer, but
 *          methods that use size and last element will
 *          work slowly(linearly).
 *      2) Fast size:
 *          sizeof this class equals sizeof(void*) + sizeof(size_t),
 *          enables access to size for a constant time,
 *      3) Fast last element:
 *          sizeof this class equals 2*sizeof(void*),
 *          last element access is constant.
 *      4) Fast size and last element:
 *          modes 2 + 3
 *
 *
 * All methods provides strong/basics exception safety(all
 * strong except the move assignment)
 */
template <typename T,
          bool TrackSize = true,
          bool TrackLast = true, typename Allocator = PrimitiveAllocator>
class LinkedList {
    // TODO: refactor iterator/pointers usage

    using Node     = llist::Node<T>;
    using BaseNode = llist::BaseNode;

    using AllocTraits = AllocatorTraits<Allocator>;

    // Used for optional fields. See the end of the definition.
    struct Empty {};
    // [[no_unique_address]] must differentiate
    // fileds of the same type. In this case it is not needed because wastes memory
    struct Empty2 {};

public:
    using value_type      = T;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using size_type       = size_t;
    using allocator_type  = Allocator;
    using iterator        = detail::LListIter<T, false>;
    using const_iterator  = detail::LListIter<T, true>;

    LinkedList() noexcept(noexcept(Allocator())) {
        initOptionalFields();
    }

    explicit LinkedList(const Allocator& alloc)
        : alloc_(alloc) {
        initOptionalFields();
    }

    /**
     * @brief Constructs `count` elements with given value.
     *
     * Allocates and copy-constructs `count` elements one by one.
     * Actually copies, then moves each element. So, `count` copies and
     * moves are performed.
     *
     * @param count Number of elements to construct
     * @param value Value to initilize elements
     * @param alloc Container's allocator
     *
     * @throws Provides strong exception guarantee.
     */
    explicit LinkedList(size_t count, const value_type& value = value_type(),
                        const Allocator& alloc = Allocator())
        : LinkedList(alloc) {
        // Thanks to delegeting c-tor, if exception is thrown,
        // the destructor will be called automatically
        auto cur_it = cbeforeBegin();
        for (; count > 0; --count, ++cur_it) {
            emplaceAfter(cur_it, value);
        }
    }

    /**
     * @brief Initialize list with the given range.
     *
     * Copies all elements in `[first, last)`.
     * Actually copies, then moves each element.
     *
     * @param first Start of the range
     * @param last End of the range
     * @param alloc Container's allocator
     *
     * @throws Provides strong exception guarantee.
     */
    template <InputIterator Iter>
    LinkedList(Iter first, Iter last,
               const Allocator& alloc = Allocator())
        : LinkedList(alloc) {
        // Thanks to delegeting c-tor, if exception is thrown,
        // the destructor will be called automatically
        auto cur = beforeBegin();
        for (; first != last; ++first) {
            emplaceAfter(cur, *first);
            ++cur;
        }
    }

    /**
     * @brief Initialize list with the given initializer_list.
     *
     * @param init_list Initializer list to copy
     * @param alloc Container's allocator
     *
     * @throws Provides strong exception guarantee.
     */
    LinkedList(std::initializer_list<value_type> init_list,
               const Allocator&                  alloc = Allocator())
        : LinkedList(init_list.begin(), init_list.end(), alloc) {}

    /**
     * @brief Copy constructor
     *
     * @throws Provides strong exception guarantee
     */
    LinkedList(const LinkedList& other)
        : LinkedList(other.cbegin(),
                     other.cend(), other.alloc_) {}

    /**
     * @brief Move constructor.
     * No iterators are invalidated.
     */
    LinkedList(LinkedList&& other) noexcept(noexcept(Allocator(move(other.alloc_))))
        : head_(other.head_)
        , size_(other.size_)
        , last_(other.last_)
        , alloc_(move(other.alloc_)) {
        other.head_.next = nullptr;
        if constexpr (TrackSize) other.size_ = 0;
        if constexpr (TrackLast) other.last_ = &other.head_;
    }

    /**
     * @brief Copy/move assignment operator.
     *
     * No iterators are invalidated.
     *
     * @throws Provides conditional exception safety.
     * If operator is used as a copy operator, strong guarantee.
     * If operator is used as a move operator and interanal call
     * to `swap` throws(due to allocator), basic guarantee, since
     * moved value will be destroyed.
     */
    LinkedList& operator=(LinkedList other) {
        swap(other);
        return *this;
    }

    ~LinkedList() { clear(); }

    /**
     * @brief Effectively swaps content of the lists.
     *
     * No iterators are invalidated.
     *
     * @param other Lists to swap with
     *
     * @throws Same as swap for allocators
     */
    void swap(LinkedList& other) {
        using bmb::swap;
        // First, swap allocs to provide strong safety
        swap(alloc_, other.alloc_);

        // Head never points to itself: either node in heap or nullptr,
        // so it is safe just swap it.
        swap(head_, other.head_);
        if constexpr (TrackSize) swap(size_, other.size_);
        if constexpr (TrackLast) swap(last_, other.last_);
    }

    /**
     * @brief Removes all elements in the list.
     *
     * After this operation, there are no elements in the list,
     * `size()=0`, `beforeBegin()=beforeEnd()`.
     *
     * All iterators are invalidated.
     *
     * Time complexity: `O(n)` where n=size().
     *
     * @throws noexcept
     */
    void clear() noexcept {
        eraseAfter(cbeforeBegin(), cend());
    }

    /**
     * @brief Creates element and places it after the given iterator.
     *
     * Actually calls `emplaceAfter(pos, bmb::move(value))`. See its
     * docs for details.
     *
     * @param pos Position to insert after
     * @param value Value to initialize new node
     *
     * Time complexity: `O(1)`.
     *
     * @return iterator to the created element
     *
     * @throws Provides strong exception guarantee
     */
    iterator insertAfter(const_iterator pos, value_type value) {
        return emplaceAfter(pos, move(value));
    }

    /**
     * @brief Inserts elements from initializer list in the list.
     *
     * Actually calls `insertAfter(pos, init_list.begin(), init_list.end())`.
     * See its docs for details.
     *
     * Time complexity: `O(n)` where n=number elements to insert.
     *
     * @param pos Iterator to place elements after
     * @param init_list Initializer list to copy
     *
     * @return iterator to the last element inserted, or `pos` if init_list is empty.
     *
     * @throws Provides strong exception guarantee.
     */
    iterator insertAfter(const_iterator pos, std::initializer_list<T> init_list) {
        return insertAfter(pos, init_list.begin(), init_list.end());
    }

    /**
     * @brief Inserts elements from `[first, last)` in the list
     * after `pos`.
     *
     * That's it, copies elements in `[first, last)` and places
     * them between `pos` and `next(pos)`.
     *
     * `pos` must be valid iterator in `[beforeBegin(), end())`.
     * Otherwise UB.
     *
     * No iterators are invalidated.
     *
     * Time complexity: `O(distance(first, last))` - number elements to insert.
     *
     * @param pos Iterator to place range after
     * @param first Start of range
     * @param last End of range
     *
     * @return iterator to the last element inserted, or `pos` if `first==last`
     *
     * @throws Provides strong exception guarantee.
     */
    template <InputIterator Iter>
    iterator insertAfter(const_iterator pos, Iter first, Iter last) {
        // Constructor provides strong exception guarantee.
        // If we used  emplaceAfter(...) for each iterator [first, last),
        // it would be more complex to provide exception guarantee here.
        LinkedList tmp(first, last, getAllocator());

        if (tmp.isEmpty()) return pos.constCast();

        return spliceAfter(pos, move(tmp));
    }

    /**
     * @brief Creates new node from given arguments
     * and places it after the given position.
     *
     * After this operation, `next(pos)` will give an iterator
     * to the newly created node.
     *
     * `pos` must be valid iterator in `[beforeBegin(), end())`.
     * Otherwise UB.
     *
     * No iterators are invalidated.
     *
     * Time complexity: `O(1)`.
     *
     * @param pos Position to insert after
     * @param args... Arguments to forward in the element's c-tor
     *
     * @return iterator to the created element
     *
     * @throws Provides strong exception guarantee
     */
    template <typename... Args>
    iterator emplaceAfter(const_iterator pos, Args&&... args) {
        Node*     new_node   = createNode(forward<Args>(args)...);
        BaseNode* before_new = pos.node_;

        // Order of operations does matter
        new_node->next   = before_new->next;
        before_new->next = new_node;

        if constexpr (TrackSize) ++size_;

        if constexpr (TrackLast) {
            if (isIterToLast(pos)) last_ = new_node;
        }

        return iterator(new_node);
    }

    /**
     * @brief Creates new node from given arguments
     * in the start of the list.
     *
     * That's it, places the new node after `beforeBegin()`, so
     * it becomes `begin()`.
     *
     * See `emplaceAfter(const_iterator, Args&&...)` docs for details.
     *
     * Time complexity: `O(1)`.
     *
     * @param args... Arguments to forward in the element's c-tor
     *
     * @return iterator to the created element
     *
     * @throws Provides strong exception guarantee
     */
    template <typename... Args>
    reference emplaceFront(Args&&... args) {
        return *emplaceAfter(cbeforeBegin(), forward<Args>(args)...);
    }

    /**
     * @brief Creates new node from given arguments
     * in the end of the list.
     *
     * That's it, places the new node after `prev(end())`, so
     * it becomes last element in the list.
     *
     * See `emplaceAfter(const_iterator, Args&&...)` docs for details.
     *
     * Time complexity:
     *      1) If `TrackLast=true`: `O(1)`.
     *      2) If `TrackLast=false`: `O(n)` where n=size().
     *
     * @param args... Arguments to forward in the element's c-tor
     *
     * @return iterator to the created element
     *
     * @throws Provides strong exception guarantee
     */
    template <typename... Args>
    reference emplaceBack(Args&&... args) {
        return *emplaceAfter(iterator(getLastNode()),
                             forward<Args>(args)...);
    }

    /**
     * @brief Destroys element right after `pos`.
     *
     * `pos` must be valid iterator in `[cbeforeBegin(), end)`.
     * If `pos` is the last element in the list or
     * `isEmpty()==true`, does nothing.
     *
     * No iterators, except one to the erased element, are invalidated.
     *
     * @param pos Iterator to destroy after
     *
     * @return iterator to the element after destroyed one
     *
     * @throws noexcept
     */
    iterator eraseAfter(const_iterator pos) noexcept {
        BaseNode* to_destroy = pos.node_->next;

        if (to_destroy == nullptr) return pos.constCast();

        BaseNode* after_destroyed = to_destroy->next;

        pos.node_->next = after_destroyed;

        destroyNode(castToNode(to_destroy));

        if constexpr (TrackSize) --size_;

        if constexpr (TrackLast) {
            // Now `pos` becomes the last element in the list
            if (last_ == to_destroy) last_ = pos.node_;
        }

        return iterator(after_destroyed);
    }

    /**
     * @brief Destroys elements in `(first, last)`.
     *
     * That's it, all elements from `next(first)` to `prev(last)` are
     * erased. And `next(first)` becomes equal to `last`.
     *
     * `first` and `last` must be valid iterators in `[beforeBegin(), end()]`.
     * Otherwise UB.
     *
     * No iterators, except ones to the erased elements, are invalidated.
     *
     * If `first==last` or `next(first)==last`, does nothing.
     *
     * @param first Iterator to start the erase after
     * @param last Iterator to end the erase before
     *
     * @return `last`
     *
     * @throws noexcept
     */
    iterator eraseAfter(const_iterator first, const_iterator last) noexcept {
        if (first == last) return last.constCast();

        while (next(first) != last) {
            eraseAfter(first);
        }
        return last.constCast();
    }

    /**
     * @brief Destroys first element in the list.
     *
     * Actually calls `eraseAfter(cbeforeBegin())`. See
     * its docs for details.
     *
     * @throws noexcept
     */
    void popFront() noexcept { eraseAfter(cbeforeBegin()); }

    /**
     * @brief Destroys last element in the list.
     *
     * If `isEmpty()==true`, does nothing.
     *
     * Finds element before the last in the list and calls
     * `eraseAfter` on it. By construction, to find node before the last
     * one we need to iterate over the whole list.
     *
     * No iterators, except one to the erased element, are invalidated.
     *
     * Time complexity: `O(n)` where n=size().
     *
     * @throws noexcept
     */
    void popBack() noexcept {
        // Make sure list is not empty.
        if (begin() == end()) return;

        // Find before the last
        BaseNode* before_last = &head_;
        while (before_last->next->next != nullptr) before_last = before_last->next;

        eraseAfter(const_iterator(before_last));
    }

    /// See `merge(LinkedList&&, auto)`
    template <typename Cmp = less>
    void merge(LinkedList& other, Cmp cmp = Cmp()) {
        merge(move(other), move(cmp));
    }

    /**
     * @brief Merges elements from 2 sorted lists into one sorted.
     *
     * Transfers all elements from `other` to `this`, so `this`
     * is sorted. `other` becomes empty after this operation.
     *
     * Assumes that `this` and `other` are already sorted according to `cmp`.
     * If they don't, resulting list will be unpredictably merged.
     * Current implementation is NOT stable.
     *
     *
     * `this->getAllocator()` must be equal to `other.getAllocator()`.
     * Otherwise exception is thrown.
     *
     * If `other` refers to `this`, does nothing.
     *
     * No iterators are invalidated.
     *
     * Time complexity: `O(max(this->size(), other.size()))`.
     *
     * @param other List to merge with
     * @param cmp Comparator to use
     *
     * @throws Provides conditional exception safety.
     * If allocators are different - throws `std::runtime_error`.
     * If a call to `cmp` throws - no exception guarantee, there will be memory leak.
     * Otherwise noexcept.
     */
    template <typename Cmp = less>
    void merge(LinkedList&& other, Cmp cmp = Cmp()) {
        // NOTE: If you want, you can try to make merge at least
        // basic exception safety. Just save the last node when
        // node should be linked to the node from other list.
        // If there is exception, link these "lost" nodes to the
        // end of any list - they will be destroyed by destructor.
        //
        // Also if you may make merge be stable.

        if (this == &other) return;

        if (getAllocator() != other.getAllocator())
            throw std::runtime_error(
                "Try to merge linked list "
                "allocated via different allocator");

        // Just to make comparasion less verbose
        auto compare = [this, &cmp](auto a, auto b) -> bool {
            return cmp(castToNode(a)->val, castToNode(b)->val);
        };

        auto a    = head_.next;
        auto b    = other.head_.next;
        auto prev = &head_;

        while (a && b) {
            // Make `a` always less than `b`
            if (!compare(a, b)) bmb::swap(a, b);

            prev->next = a;
            prev       = a;
            a          = a->next;
        }
        prev->next = a != nullptr ? a : b;

        other.head_.next = nullptr;

        if constexpr (TrackSize) {
            size_ += other.size_;
            other.size_ = 0;
        }

        if constexpr (TrackLast) {
            // If nodes from `other` > than from `this`,
            // they will be placed after `last_`, so update it.
            // Remember to handle cases with empty lists.

            // `this` is empty, `other` is not
            if (last_ == &head_
                && other.last_ != &other.head_) last_ = other.last_;
            // `this` is not empty, `other` is not empty,
            // and nodes from `other` are after `this->last_`
            else if (last_ != &head_
                     && other.last_ != &other.head_
                     && compare(last_, other.last_)) {
                last_ = other.last_;
            }

            other.last_ = &other.head_;
        }
    }

    /// See `spliceAfter(const_iterator, LinkedList&&)`
    iterator spliceAfter(const_iterator pos, LinkedList& other) {
        return spliceAfter(pos, move(other));
    }

    /**
     * @brief Transfers all elements from `other` to `*this`. `other`
     * becomes empty.
     *
     * `this->getAllocator()` must be equal to `other.getAllocator()`.
     * Otherwise exception is thrown.
     *
     * `pos` must be valid iterator in `[beforeBegin(), end())`.
     * Otherwise UB.
     *
     * If `other` refers to `*this`, does nothing.
     *
     * No iterators are invalidated.
     *
     * Time complexity:
     *      1) If `TrackLast==true`: `O(1)`.
     *      2) If `TrackLast==false`: `O(other.size())`.
     *
     * @param pos Iterator to transfer after
     * @param other List to transfer from
     *
     * @return iterator to the last element transferred or `pos`
     *
     * @throws std::runtime_error if allocators aren't equal
     */
    iterator spliceAfter(const_iterator pos, LinkedList&& other) {
        if (this == &other || other.isEmpty()) return pos.constCast();

        if (getAllocator() != other.getAllocator())
            throw std::runtime_error(
                "Try to transfer linked list "
                "allocated via different allocator");

        // If track last, we can bypass its search.
        if constexpr (TrackLast) {
            auto last_transferred = other.last_;

            transferAfter(pos.node_,
                          &other.head_, other.last_);

            if (isIterToLast(pos)) last_ = other.last_;

            other.last_ = &other.head_;

            // Update size
            if constexpr (TrackSize) {
                size_ += other.size_;
                other.size_ = 0;
            }

            return iterator(last_transferred);
        }

        // If `last_` is not available, use non effective version
        return spliceAfter(pos, move(other),
                           other.beforeBegin(), other.end());
    }

    /// See `spliceAfter(const_iterator, LinkedList&&, const_iterator, const_iterator)`
    iterator spliceAfter(const_iterator pos, LinkedList& other,
                         const_iterator first, const_iterator last) {
        return spliceAfter(pos, move(other), first, last);
    }

    /**
     * @brief Transfers all elements in `(first, last)` from `other`
     * to `*this` right after `pos`.
     *
     * If `first==last` or `next(first)==last`, does nothing and
     * returns `pos`.
     *
     * `other` can refer to `*this`. In this case `pos` must
     * not be in (first, last), otherwise UB - at least memory leak.
     *
     * `this->getAllocator()` must be equal to `other.getAllocator()`.
     * Otherwise exception is thrown.
     *
     * `pos` must be valid iterator in `[beforeBegin(), end())`.
     * Otherwise UB.
     *
     * No iterators are invalidated.
     *
     * Time complexity: `O(distance(first, last))`.
     *
     * @param pos Iterator to transfer range after
     * @param other List to transfer from
     * @param first Iterator before the element to start cut
     * @param last Iterator after the element to stop cut
     *
     * @return iterator to the last element transferred or `pos`
     *
     * @throws std::runtime_error if allocators aren't equal
     */
    iterator spliceAfter(const_iterator pos, LinkedList&& other,
                         const_iterator first, const_iterator last) {
        if (getAllocator() != other.getAllocator())
            throw std::runtime_error(
                "Try to transfer linked list nodes "
                "allocated via different allocator");

        // If no elements to transfer
        if (first == last
            || next(first) == last) return pos.constCast();

        // Find last element to transfer(right before `last` iterator)
        auto   before = first.node_;
        auto   end    = before;
        size_t cnt    = 0;

        while (end->next != last.node_) {
            end = end->next;
            ++cnt;
        }

        if constexpr (TrackSize) {
            // Update only if obtained new elements
            if (this != &other) {
                size_ += cnt;
                other.size_ -= cnt;
            }
        }

        if constexpr (TrackLast) {
            if (isIterToLast(pos)) last_ = end;
        }

        return iterator(transferAfter(pos.node_, before, end));
    }

    /**
     * @brief Reverses linked list.
     *
     * Time complexity: `O(n)` where n=size()
     *
     * No iterators, except one to the erased element, are invalidated.
     *
     * @throws noexcept
     */
    void reverse() noexcept {
        BaseNode* left  = nullptr;
        BaseNode* right = head_.next;
        // Left will be point to the new first element
        while (right != nullptr) {
            auto keep   = right->next;
            right->next = left;
            left        = right;
            right       = keep;
        }

        if constexpr (TrackLast) {
            if (!isEmpty()) last_ = head_.next;
            // If empty - last points to the `head`
        }

        head_.next = left;
    }

    /**
     * @brief Returns first element in the list.
     *
     * If list is empty, UB.
     */
    reference front() noexcept { return *begin(); }

    /**
     * @brief Returns first element in the list.
     *
     * If list is empty, UB.
     */
    const_reference front() const noexcept { return *begin(); }

    /**
     * @brief Returns last element in the list.
     *
     * Time complexity:
     *      1) If `TrackLast=true`: `O(1)`.
     *      2) If `TrackLast=false`: `O(n)` where n=size().
     *
     * If list is empty, UB.
     */
    reference back() noexcept {
        return *iterator(getLastNode());
    }

    /**
     * @brief Returns last element in the list.
     *
     * Time complexity:
     *      1) If `TrackLast=true`: `O(1)`.
     *      2) If `TrackLast=false`: `O(n)` where n=size().
     *
     * If list is empty, UB.
     */
    const_reference back() const noexcept {
        return *iterator(getLastNode());
    }

    /**
     * @brief Returns raw representation of
     * the first list's node.
     *
     * Generally, you should not use it.
     */
    Node* getRawFirstNode() const noexcept {
        return castToNode(begin().node_);
    }

    /**
     * @brief Returns list's size.
     *
     * Time complexity:
     *      1) If `TrackSize=true`: `O(1)`.
     *      2) If `TrackSize=false`: `O(n)` where n - num. of elements in list.
     *
     * @return number of elements in the list
     *
     * @throws noexcept
     */
    size_t size() const noexcept {
        if constexpr (TrackSize) return size_;

        size_t    cnt = 0;
        BaseNode* cur = &head_;
        while (cur->next != nullptr) {
            ++cnt;
            cur = cur->next;
        }
        return cnt;
    }

    /**
     * @brief Checks whether list has elements.
     *
     * @return true, if list is empty. Otherwise false.
     *
     * @throws noexcept
     */
    bool isEmpty() const noexcept {
        // Don't use `size()` here, since
        // it may has linear time complexity
        return head_.next == nullptr;
    }

    allocator_type getAllocator() const { return alloc_; }

    iterator begin() noexcept { return iterator(head_.next); }
    iterator end() noexcept { return iterator(nullptr); }

    const_iterator begin() const noexcept { return const_iterator(head_.next); }
    const_iterator end() const noexcept { return const_iterator(nullptr); }

    const_iterator cbegin() const noexcept { return const_iterator(head_.next); }
    const_iterator cend() const noexcept { return const_iterator(nullptr); }

    iterator       beforeBegin() noexcept { return iterator(&head_); }
    const_iterator beforeBegin() const noexcept { return const_iterator(&head_); }
    const_iterator cbeforeBegin() const noexcept { return const_iterator(&head_); }

    iterator       beforeEnd() noexcept { return iterator(getLastNode()); }
    const_iterator beforeEnd() const noexcept { return const_iterator(getLastNode()); }
    const_iterator cbeforeEnd() const noexcept { return const_iterator(getLastNode()); }
    // reverese_iterator rbegin() ....

    bool operator==(const LinkedList& other) const noexcept {
        if constexpr (TrackSize) {
            return size_ == other.size_
                   && equal(begin(), end(), other.begin());
        }
        return equal(begin(), end(), other.begin(), other.end());
    }

    auto operator<=>(const LinkedList& other) const noexcept {
        return lexicographical_compare_three_way(begin(), end(),
                                                 other.begin(), other.end());
    }

private:
    /**
     * @brief Effectively transfers nodes
     * from `(begin, end]` to right after `pos`.
     * That's it, `end` must be valid node in the list.
     *
     * Does NOT update list's `size_` and `last_` if needed.
     *
     * `pos` must be valid node in `[beforeBegin(), end())`.
     *
     * Time complexity: `O(1)`.
     *
     * @param pos Pointer to insert elements after
     * @param begin Pointer right before the start of the range
     * @param end Pointer to the last element to be transferred
     *
     * @return `end`
     *
     * @throws noexcept
     */
    BaseNode* transferAfter(BaseNode* pos,
                            BaseNode* begin, BaseNode* end) noexcept {
        // By construction, linked list must know the element
        // before the first one to transfer, since we have to update
        // next pointer for it. Also we should know the last element
        // to transfer for proper pointers rearrangment.

        auto keep = begin->next;

        begin->next = end->next;
        end->next   = pos->next;

        pos->next = keep;
        return end;
    }

    Node* castToNode(BaseNode* ptr) const noexcept { return static_cast<Node*>(ptr); }

    void initOptionalFields() noexcept {
        if constexpr (TrackSize) size_ = 0;
        if constexpr (TrackLast) last_ = &head_;
    }

    /**
     * @brief Allocates and constructs a Node with given args.
     *
     * @param args... Args to forward in constructor
     * @return Pointer to the constructed Node
     *
     * @throws Provides strong exception guarantee.
     */
    template <typename... Args>
    Node* createNode(Args&&... args) {
        auto node = AllocTraits::template allocate<Node>(alloc_, 1);
        try {
            AllocTraits::construct(alloc_, node, forward<Args>(args)...);
        } catch (...) {
            AllocTraits::deallocate(alloc_, node, 1);
            throw;
        }
        return node;
    }

    /**
     * @brief Destroys and deallocates given node.
     *
     * @param node Pointer to node to destroy
     *
     * @throws noexcept
     */
    void destroyNode(Node* node) noexcept {
        AllocTraits::destroy(alloc_, node);
        AllocTraits::deallocate(alloc_, node, 1);
    }

    /**
     * @brief Checks whether given iterator points
     * to the last node in the list.
     *
     * Time complexity:
     *      If `TrackLast=true`: `O(1)`.
     *      If `TrackLast=false`: `O(n)` where n=size()
     *
     * @param it Iterator to check.
     * @return true, if iterator is the last node. Otherwise, false.
     *
     * @throws noexcept
     */
    bool isIterToLast(const_iterator it) const noexcept {
        return it == const_iterator(getLastNode());
    }

    /**
     * @brief Obtains last node in the list.
     *
     * If `TrackLast=true`, uses cached value.
     * In this case, if the list was modified before call,
     * cached value must be updated manually,
     * otherwise the method may return wrong node.
     *
     * If `TrackLast=false` visits every node in the list.
     *
     * Time complexity:
     *      If `TrackLast=true`: `O(1)`.
     *      If `TrackLast=false`: `O(n)` where n=size().
     *
     * @return Pointer to the last node, or pointer
     * to the head, if the list is empty.
     *
     * @throws noexcept
     */
    BaseNode* getLastNode() const noexcept {
        if constexpr (TrackLast) return last_;

        BaseNode* cur = &head_;
        while (cur->next != nullptr) cur = cur->next;
        return cur;
    }

    // Make mutable for simplicity of const conversions.
    // Actually needed for const_iterator(&head) for const lists.
    // But const_iterator can be constructed only from non-const
    // BaseNodes.
    mutable BaseNode head_;

    // Enables fast access to the list's size.
    // If don't track size, use empty struct that
    // highly(and should in this case) will be optimized and will not use memory.
    [[no_unique_address]]
    conditional_t<TrackSize, size_t, Empty> size_;

    // Enables fast access to the last element in the list.
    // Invariant: if list is empty, points to the `head_`, otherwise to the real Node.
    [[no_unique_address]]
    conditional_t<TrackLast, BaseNode*, Empty2> last_;

    [[no_unique_address]] Allocator alloc_;
};

/// LinkedList with enabled track of size.
template <typename T, bool TrackLast,
          typename Allocator = PrimitiveAllocator>
using LListFastSize = LinkedList<T, true, TrackLast, Allocator>;

/// LinkedList with enabled track of last element.
template <typename T, bool TrackSize,
          typename Allocator = PrimitiveAllocator>
using LListFastLast = LinkedList<T, TrackSize, true, Allocator>;

/// LinkedList without size and last element tracking.
template <typename T,
          typename Allocator = PrimitiveAllocator>
using LListTiny = LinkedList<T, false, false, Allocator>;

/// LinkedList with size and last element tracking.
template <typename T,
          typename Allocator = PrimitiveAllocator>
using LListFat = LinkedList<T, true, true, Allocator>;

/// See `LinkedList::swap`
template <typename T,
          bool TrackSize = true,
          bool TrackLast = true, typename Allocator = PrimitiveAllocator>
void swap(LinkedList<T, TrackSize, TrackLast, Allocator>& a,
          LinkedList<T, TrackSize, TrackLast, Allocator>& b) {
    a.swap(b);
}

template <InputIterator Iter, typename Allocator = PrimitiveAllocator>
LinkedList(Iter, Iter, Allocator = Allocator()) -> LinkedList<typename IteratorTraits<Iter>::value_type,
                                                              true, true, PrimitiveAllocator>;

}  // namespace bmb
