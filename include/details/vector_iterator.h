#pragma once
/**
 * @file vector_iterator
 * @warning This file is an internal implementation details and should not be
 * directly used. Implementation of Vector's iterator
 * @authors bivafra
 */

#include <cstddef>

#include "utils/iterators.h"
#include "utils/type_traits.h"

namespace bmb {
namespace detail {

/**
 * @class BaseVectorIter
 * @brief contiguous iterator for Vector
 * Trivial iterator-wrapper for pointer to array(aka T*)
 */
template <typename T, bool IsConst>
class BaseVectorIter {
private:
    using Self = BaseVectorIter;

public:
    using value_type        = remove_const_t<T>;
    using reference         = conditional_t<IsConst, const T, T>&;
    using pointer           = conditional_t<IsConst, const T, T>*;
    using difference_type   = ptrdiff_t;
    using iterator_category = contiguous_iter_tag;

    BaseVectorIter()
        : ptr_(nullptr) {};

    explicit BaseVectorIter(pointer ptr)
        : ptr_(ptr) {};

    // Want a conversion non-const -> const underlying value
    operator BaseVectorIter<T, true>() const { return BaseVectorIter<T, true>(ptr_); }

    pointer   operator->() const { return ptr_; }
    reference operator*() const { return *ptr_; }

    Self& operator++() {
        ++ptr_;
        return *this;
    }

    Self operator++(int) {
        Self copy = *this;
        ++ptr_;
        return copy;
    }

    Self& operator+=(difference_type n) {
        ptr_ += n;
        return *this;
    }

    Self operator+(difference_type n) {
        return Self{ptr_ + n};
    }

    friend Self operator+(difference_type n, const Self& it) {
        return Self{it.ptr_ + n};
    }

    Self& operator--() {
        --ptr_;
        return *this;
    }

    Self operator--(int) {
        Self copy = *this;
        --ptr_;
        return copy;
    }

    Self& operator-=(difference_type n) {
        ptr_ -= n;
        return *this;
    }

    Self operator-(difference_type n) { return {ptr_ - n}; }

    friend difference_type operator-(const Self& lhs,
                                     const Self& rhs) { return lhs.ptr_ - rhs.ptr_; }

    // NOTE: consider spaceship opeator
    bool operator<(const Self& rhs) { return ptr_ < rhs.ptr_; }

    bool operator>(const Self& rhs) { return rhs < *this; }

    bool operator<=(const Self& rhs) { return !(*this > rhs); }

    bool operator>=(const Self& rhs) { return !(*this < rhs); }

    bool operator==(const Self& rhs) { return ptr_ == rhs.ptr_; }

    bool operator!=(const Self& rhs) { return ptr_ != rhs.ptr_; }

private:
    pointer ptr_;
};

}  // namespace detail
}  // namespace bmb
