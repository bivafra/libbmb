#pragma once

/**
 * @file iterators.h
 * Provides iterator tags, concepts, and iterator adaptors
 * @authors bivafra
 */

#include "utils/concepts.h"

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

// TODO: improve these concepts. Now they aren't as general as 'std::' ones.
// Integrate output iterator with input iterator.
// NOTE: for info about requirements see:
// https://en.cppreference.com/w/cpp/named_req/Iterator.

/// InputIterator
template <typename Iter>
concept InputIterator = requires(Iter it) {
    { ++it } -> SameAs<Iter&>;
    { *it } -> ConvertibleTo<typename Iter::value_type>;
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
      && requires(Iter i, Iter j, Iter::difference_type n) {
             { i += n } -> SameAs<Iter&>;
             { i -= n } -> SameAs<Iter&>;

             { i + n } -> SameAs<Iter>;
             { n + i } -> SameAs<Iter>;

             { i - n } -> SameAs<Iter>;

             { i - j } -> SameAs<typename Iter::difference_type>;

             { i < j } -> ConvertibleTo<bool>;
             { i > j } -> ConvertibleTo<bool>;
             { i <= j } -> ConvertibleTo<bool>;
             { i >= j } -> ConvertibleTo<bool>;
         };

}  // namespace bmb
