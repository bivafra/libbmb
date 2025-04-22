#pragma once

/**
 * @file iterators.h
 * Provides iterator tags, concepts, and iterator adaptors
 * @authors bivafra
 */

#include <cstddef>

#include "utils/concepts.h"
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

/// IteratorTraits
template <typename Iter>
struct IteratorTraits {
    using value_type        = Iter::value_type;
    using reference         = Iter::reference;
    using pointer           = Iter::pointer;
    using difference_type   = Iter::difference_type;
    using iterator_category = Iter::iterator_category;
};

template <typename T>
struct IteratorTraits<T*> {
    using value_type        = remove_const_t<T>;
    using reference         = T&;
    using pointer           = T*;
    using difference_type   = ptrdiff_t;
    using iterator_category = contiguous_iter_tag;
};

// TODO: improve these concepts. Now they aren't as general as 'std::' ones.
// Integrate output iterator with input iterator.
// NOTE: for info about requirements see:
// https://en.cppreference.com/w/cpp/named_req/Iterator.

/// InputIterator
template <typename Iter>
concept InputIterator = requires(Iter it) {
    { ++it } -> SameAs<Iter&>;
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

}  // namespace bmb
