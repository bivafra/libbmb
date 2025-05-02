#pragma once
/**
 * @file algo_base.h
 * File provides basic, convenient functions (like equal, transform, etc.)
 * @authors bivafra
 */

#include <compare>

#include "utils/compare.h"
#include "utils/concepts.h"
#include "utils/iterators.h"

namespace bmb {

/**
 * @brief Compares ranges lexicographically using C++20 three-way comparation.
 *
 * Compares elements iteratively, until either non-equal were found
 * or some range was exausted. In the 2-nd case, detects longer range as greater.
 *
 * Time complexity: O(min(last1 - first1, last2 - first2))
 *
 * @param first1 - Start of the 1-st range
 * @param last1 - End of the 1-st range
 * @param first2 - Start of the 2-nd range
 * @param last2 - End of the 2-nd range
 * @param cmp - Comparator that behaves as <=>. 'compare_three_way' is default one.
 *
 * @return The relation of the ranges(like std::strong_ordering::less/greater/equal, etc.)
 *
 * @throws Almost always noexcept. In fact, the same specification as
 * iterators operations and call of comparator.
 */
template <InputIterator Iter1,
          InputIterator Iter2, typename Cmp = compare_three_way>
auto lexicographical_compare_three_way(Iter1 first1, Iter1 last1,
                                       Iter2 first2, Iter2 last2,
                                       Cmp cmp = Cmp()) {
    while (first1 != last1
           && first2 != last2) {
        if (auto c = cmp(*first1, *first2); c != 0) return c;

        ++first1, ++first2;
    }

    return first1 != last1   ? std::strong_ordering::greater
           : first2 != last2 ? std::strong_ordering::less
                             : std::strong_ordering::equal;
}

/**
 * @brief Checks whether given ranges are equal.
 *
 * More precisely, compares (last1 - first1) elements from 2-nd range.
 * Therefore, 1-st range must not shorter than 2-nd one.
 * Otherwise undefined behaviour - will increment the end iterator from 2-nd range.
 * Given Predicate must return boolean-convertible value.
 *
 * @param first1 Start of the 1-st range
 * @param last1 End of the 1-st range
 * @param first2 Start of the 2-nd range
 * @param pred Predicate that compares range elements
 *
 * @return true if ranges are equal on Predicate. false otherwise.
 *
 * @throws Almost always noexcept. In fact, the same specification
 * as operations on iterators and calling predicate.
 */
template <InputIterator Iter1,
          InputIterator Iter2, typename Pred = equal_to>
    requires Predicate<Pred,
                       typename IteratorTraits<Iter1>::value_type,
                       typename IteratorTraits<Iter2>::value_type>
bool equal(Iter1 first1, Iter1 last1, Iter2 first2, Pred pred = Pred()) {
    while (first1 != last1) {
        if (!pred(*first1, *first2)) return false;

        ++first1, ++first2;
    }
    return true;
}

}  // namespace bmb
