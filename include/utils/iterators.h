#pragma once

/**
 * @file iterators.h
 * Provides iterator tags, concepts, and iterator adaptors
 * @authors bivafra
 */

#include <cstddef>

#include "utils/concepts.h"
#include "utils/move.h"
#include "utils/type_traits.h"

namespace bmb {

/// input_iter_tag
struct input_iter_tag {};
/// output_iter_tag
struct output_iter_tag {};
/// forward_iter_tag
struct forward_iter_tag : public input_iter_tag {};
/// bidirectional_iter_tag
struct bidirectional_iter_tag : public forward_iter_tag {};
/// random_access_iter_tag
struct random_access_iter_tag : public bidirectional_iter_tag {};
/// contiguous_iter_tag
struct contiguous_iter_tag : public random_access_iter_tag {};

namespace detail {
// NOTE: this makes IteratorTraits SFINAE friendly

template <typename Iter, typename = void_t<>>
struct IteratorTraitsImpl {};

template <typename Iter>
struct IteratorTraitsImpl<Iter, void_t<typename Iter::value_type,
                                       typename Iter::reference,
                                       typename Iter::pointer,
                                       typename Iter::difference_type,
                                       typename Iter::iterator_category>> {
    using value_type        = Iter::value_type;
    using reference         = Iter::reference;
    using pointer           = Iter::pointer;
    using difference_type   = Iter::difference_type;
    using iterator_category = Iter::iterator_category;
};

}  // namespace detail

/// IteratorTraits
template <typename Iter>
struct IteratorTraits : public detail::IteratorTraitsImpl<Iter> {};

template <typename T>
struct IteratorTraits<T*> {
    using value_type        = remove_const_t<T>;
    using reference         = T&;
    using pointer           = T*;
    using difference_type   = ptrdiff_t;
    using iterator_category = contiguous_iter_tag;
};

// TODO: improve these concepts. Now they aren't as general as 'std::' ones.
// NOTE: for info about requirements see:
// https://en.cppreference.com/w/cpp/named_req/Iterator.

/// InputIterator
template <typename Iter>
concept InputIterator = requires(Iter it) {
    { ++it } -> SameAs<Iter&>;
    { *it } -> ConvertibleTo<typename IteratorTraits<Iter>::value_type>;
};

/// OutputIterator
template <typename Iter, typename T>
concept OutputIterator = requires(Iter it, T&& t) {
    *it++ = forward<T>(t);
    { *it } -> ConvertibleTo<typename IteratorTraits<Iter>::value_type>;
};

/// ForwardIterator
template <typename Iter>
concept ForwardIterator = InputIterator<Iter> && requires(Iter it) {
    { it++ } -> SameAs<Iter>;
};

/// BidirectionalIterator
template <typename Iter>
concept BidirectionalIterator = ForwardIterator<Iter> && requires(Iter it) {
    { --it } -> SameAs<Iter&>;
    { it-- } -> SameAs<Iter>;
};

/// RandomAccessIterator
template <typename Iter>
concept RandomAccessIterator
    = BidirectionalIterator<Iter>
      && requires(Iter i, Iter j, IteratorTraits<Iter>::difference_type n) {
             { i += n } -> SameAs<Iter&>;
             { i -= n } -> SameAs<Iter&>;

             { i + n } -> SameAs<Iter>;
             { n + i } -> SameAs<Iter>;

             { i - n } -> SameAs<Iter>;

             { i - j } -> SameAs<typename IteratorTraits<Iter>::difference_type>;

             { i < j } -> ConvertibleTo<bool>;
             { i > j } -> ConvertibleTo<bool>;
             { i <= j } -> ConvertibleTo<bool>;
             { i >= j } -> ConvertibleTo<bool>;
         };

/// ContiguousIterator
template <typename Iter>
concept ContiguousIterator = RandomAccessIterator<Iter>;

/**
 * @brief Returns the number of elements in [first, last).
 *
 * Time complexity: O(1) if Iter satisfies at least RandomAccessIterator,
 * otherwise O(last - first).
 *
 * @param first Iterator to the first element
 * @param last Iterator to the end of the range
 *
 * @return The number of increments needed to go from first to last
 *
 * @throws Almost always nothrow. In general, the same as
 * either operator-(It, It) or operator++()
 */
template <InputIterator Iter>
auto distance(Iter first, Iter last) -> IteratorTraits<Iter>::difference_type {
    using diff_t        = IteratorTraits<Iter>::difference_type;
    using iter_category = IteratorTraits<Iter>::iterator_category;

    // If can - do it for O(1), otherwise iterate through all range
    if constexpr (is_base_of_v<random_access_iter_tag, iter_category>) {
        return last - first;
    }

    diff_t n = 0;
    while (first != last) {
        ++first;
        ++n;
    }
    return n;
}

/**
 * @brief Increments given iterator by dist.
 *
 * If iterator is at least RandomAccessIterator - uses +=.
 * If it BidirectionalIterator - uses either + or - dist times.
 * If it at least InputIterator and dist > 0 - uses +, if dist < 0 - has no effect.
 *
 * Time complexity: O(1) for RandomAccessIterator, otherwise O(dist)
 *
 * @param it Iterator to be advanced
 * @param dist Number of elements iterator should be advanced
 *
 * @throws Almost always nothrow. In general, the same as
 * modifying iterator operations.
 */
template <InputIterator Iter, typename Distance>
void advance(Iter& it, Distance dist) {
    using diff_t        = IteratorTraits<Iter>::difference_type;
    using iter_category = IteratorTraits<Iter>::iterator_category;

    auto n = static_cast<diff_t>(dist);

    // Make operations according to the iterator category
    if constexpr (is_base_of_v<random_access_iter_tag, iter_category>)
        it += n;

    else {
        while (n > 0) {
            --n;
            ++it;
        }

        if constexpr (is_base_of_v<bidirectional_iter_tag, iter_category>) {
            while (n < 0) {
                ++n;
                --it;
            }
        }
    }
}

/**
 * @brief Returns the n-th successor of given iterator.
 *
 * @param it Iterator
 * @param n Number of elements to shift the iterator
 *
 * @return Iterator that holds n-th successor of the given one
 *
 * @throws Same as advance(Iter, diff_type)
 */
template <InputIterator Iter>
Iter next(Iter it, typename IteratorTraits<Iter>::difference_type n = 1) {
    advance(it, n);
    return it;
}

/**
 * @brief Returns the n-th predecessor of given iterator.
 * If n < 0, will chose n-th successor.
 *
 * @param it Iterator
 * @param n Number of elements to shift the iterator
 *
 * @return Iterator that holds n-th predecessor of the given one
 *
 * @throws Same as advance(Iter, diff_type)
 */
template <BidirectionalIterator Iter>
Iter prev(Iter it, typename IteratorTraits<Iter>::difference_type n = 1) {
    advance(it, -n);
    return it;
}

}  // namespace bmb
