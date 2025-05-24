#pragma once
/**
 * @file linked_list.h
 * Singly linked list implementation
 * @authors bivafra
 */

#include <cstddef>
#include <initializer_list>

#include "algo/algo_base.h"
#include "details/llist_iterator.h"
#include "details/llist_nodes.h"
#include "utils/allocator.h"
#include "utils/compare.h"
#include "utils/iterators.h"
#include "utils/move.h"
#include "utils/type_traits.h"

namespace bmb {

// for simplicity merge and spliceAfter work only if allocators are the same. Why? because :)
template <typename T,
          bool TrackSize = true,
          bool TrackLast = true, typename Allocator = PrimitiveAllocator>
class LinkedList {
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

    LinkedList() noexcept(noexcept(Allocator())) {}

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
     * @param value Value to initilize elements.
     * @param alloc Container's allocator.
     *
     * @throws Provides strong exception guarantee.
     */
    LinkedList(size_t count, const value_type& value = value_type(),
               const Allocator& alloc = Allocator())
        : LinkedList(alloc) {
        auto cur_it = cbeforeBegin();

        for (; count > 0; --count, ++cur_it) {
            try {
                insertAfter(cur_it, value);
            } catch (...) {
                clear();
                throw;
            }
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
     * @param alloc Container's allocator.
     *
     * @throws Provides strong exception guarantee.
     */
    template <InputIterator Iter>
    LinkedList(Iter first, Iter last,
               const Allocator& alloc = Allocator())
        : LinkedList(alloc) {
        try {
            for (; first != last; ++first) {
                auto node = createNode(*first);

                if constexpr (TrackSize) ++size_;
                if constexpr (TrackLast) last_ = node;
            }
        } catch (...) {
            // Use the fact, that in case of exception here
            // we should destroy all elements. That
            // would be harder to implement in `insertAfter`,
            // since there we would have to track start and
            // end of the created range.
            clear();
            throw;
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
     * Implemented through copy/move-and-swap idiom.
     *
     * @throws Provides conditional exception safety.
     * If operator is used as a copy operator, strong guarantee.
     * If operator is used as a move operator and interanal call
     * to `swap` throws(due to allocator), basic guarantee, since
     * moved value will be destroyed.
     */
    LinkedList& operator=(LinkedList other) {
        swap(*this, other);
        return *this;
    }

    ~LinkedList() { clear(); }

    /**
     * @brief Effectively swaps content of the lists.
     *
     * @param other Lists to swap with
     *
     * @throws Same as swap for allocators
     */
    void swap(LinkedList& other) {
        // First, swap allocs to provide strong safety
        swap(alloc_, other.alloc_);

        swap(head_, other.head_);
        if constexpr (TrackSize) swap(size_, other.size_);
        if constexpr (TrackLast) swap(last_, other.last_);
    }  // TODO:

    /**
     * @brief Removes all elements in the list.
     *
     * After this operation, there are no elements in the list,
     * if `TrackSize=true`, then size()=0, if `TrackLast=true`,
     * it is updated appropriately.
     *
     * Time complexity: `O(size())`.
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
     * after the `pos`.
     *
     * That's it, copies elements in `[first, last)` and places
     * them between `pos` and `next(pos)`.
     *
     * `pos` must be valid iterator in `[beforeBegin(), end())`.
     * Othrwise UB.
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

        // TODO: put them in spliceAfterImpl
        //
        // if constexpr (TrackSize) size_ += tmp.size_;
        //
        // if constexpr (TrackLast) {
        //     if (isIterToLast(pos)) last_ = tmp.last_;
        // }

        // TODO: consider to use here transferAfter
        return spliceAfterImpl(pos, tmp.cbeforeBegin(), tmp.cend());
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
     * @brief Destroys element right after the `pos`.
     *
     * `pos` must be valid iterator in `[cbeforeBegin(), end)`.
     * If `pos` is the last element in the list or
     * `isEmpty()==true`, does nothing.
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
            ++first;
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
     *
     * Time complexity: `O(n)` where n=size()
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

    void merge(LinkedList& other, auto cmp = less());
    void merge(LinkedList&& other, auto cmp = less());

    /// See `spliceAfter(const_iterator, LinkedList&&)`
    void spliceAfter(const_iterator pos, LinkedList& other) {
        spliceAfter(pos, move(other));
    }

    /// See `spliceAfter(const_iterator, const_iterator, const_iterator)`
    void spliceAfter(const_iterator pos, LinkedList&& other) {
        spliceAfter(pos, other.begin(), other.end());
    }

    void spliceAfter(const_iterator pos,
                     const_iterator first, const_iterator last) {
        spliceAfterImpl(pos, first, last);
    }

    void reverse() noexcept;

    reference front() const;
    reference back() const;
    // TODO: getrawfirstnode

    /**
     * @brief Return list's size
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
        return head_.next == nullptr;
    }

    allocator_type getAllocator() const noexcept { return alloc_; }

    iterator begin() noexcept { return iterator(head_.next); }
    iterator end() noexcept { return iterator(nullptr); }

    const_iterator begin() const noexcept { return const_iterator(head_.next); }
    const_iterator end() const noexcept { return const_iterator(nullptr); }

    const_iterator cbegin() const noexcept { return const_iterator(head_.next); }
    const_iterator cend() const noexcept { return const_iterator(nullptr); }

    iterator       beforeBegin() noexcept { return iterator(&head_); }
    const_iterator beforeBegin() const noexcept { return const_iterator(&head_); }
    const_iterator cbeforeBegin() const noexcept { return const_iterator(&head_); }

    // reverese_iterator rbegin() ....

    bool operator==(const LinkedList& other) const noexcept {
        if constexpr (TrackSize) {
            return size_ == other.size_
                   && equal(begin(), end(), other.begin());
        }
        return equal(begin(), end(), other.begin());
    }

    auto operator<=>(const LinkedList& other) const noexcept {
        return lexicographical_compare_three_way(begin(), end(),
                                                 other.begin(), other.end());
    }

private:
    /**
     * @brief Transfers all elements ....
     *
     * @param Name and description of the parameter
     * @return Description of the returned value
     *
     * @throws Description of the exception safety and possible exceptions
     */
    iterator spliceAfterImpl(const_iterator pos,
                             const_iterator first, const_iterator last) noexcept {
        // If no elements to transfer
        if (first == last
            || next(first) == last) return pos.constCast();

        // Find last element to transfer(right before `last` iterator)
        auto before = first.node_;
        auto end    = before;
        while (end->next != last.node_) end = end->next;

        return iterator(transferAfter(pos.node_, before, end));
    }

    /**
     * @brief Effectively transfers nodes
     * from `(begin, end]` to right after the `pos`.
     * That's it, the range is not standard `[begin, end)`.
     * Does NOT update list's `size_` and `last_` if needed.
     *
     * `end` may equal to `nullptr`, which means the `end()`
     * element of the list.
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

        // transfer untill the end
        if (end == nullptr) {
            begin->next = nullptr;
        }
        // or cut inside list
        else {
            begin->next = end->next;
            end->next   = pos->next;
        }

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
     * In this case, if the list was modified, cached
     * value must be updated manually, otherwise
     * the method returns wrong node.
     *
     * If `TrackLast=false` visits every node in the list.
     *
     * Time complexity:
     *      If `TrackLast=true`: `O(1)`.
     *      If `TrackLast=false`: `O(n)` where n=size()
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

inline void foo() {
    LinkedList<int> arr;
    arr.emplaceAfter(arr.beforeBegin(), 5);
    struct A {
        A(int, int);
    };
    LinkedList<A> bb;
    bb.emplaceAfter(bb.beforeBegin(), 2, 3);

    arr.cbegin();
    arr.insertAfter(arr.begin(), arr.begin(), arr.end());
}

// TODO: reconsider noexcept
// TODO: iterator invalidation
// TODO: refactor iterator/pointers usage
// TODO: mention what updates size and last

}  // namespace bmb
