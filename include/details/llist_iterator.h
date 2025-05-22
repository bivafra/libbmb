#pragma once
/**
 * @file llist_nodes.h
 * LinkedList iterator definition
 * @authors bivafra
 */

#include <cstddef>

#include "details/llist_nodes.h"
#include "utils/iterators.h"
#include "utils/type_traits.h"

namespace bmb {
namespace detail {

/**
 * @class LListIter
 * @brief Forward iterator for linked list.
 * @tparam T value type that linked list node stores
 */
template <typename T, bool IsConst>
class LListIter {
    using Self = LListIter;

    // NOTE: Nodes are never const. We provide
    // const-correctness through 'reference' and 'pointer' usings
    using Node     = llist::Node<T>;
    using BaseNode = llist::BaseNode;

public:
    using value_type        = remove_const_t<T>;
    using reference         = conditional_t<IsConst, const T, T>&;
    using pointer           = conditional_t<IsConst, const T, T>*;
    using difference_type   = ptrdiff_t;
    using iterator_category = forward_iter_tag;

    LListIter() noexcept
        : node_(nullptr) {};

    explicit LListIter(BaseNode* node) noexcept
        : node_(node) {};

    // Want a conversion non-const -> const underlying type
    operator LListIter<T, true>() const noexcept { return LListIter<T, true>(node_); }

    // Sometimes want to return given const_iterator,
    // but non-const
    LListIter<T, false> constCast() const noexcept {
        return LListIter<T, false>(node_);
    }

    pointer   operator->() const noexcept { return &static_cast<Node*>(node_)->val; }
    reference operator*() const noexcept { return static_cast<Node*>(node_)->val; }

    Self& operator++() noexcept {
        node_ = node_->next;
        return *this;
    }

    Self operator++(int) noexcept {
        Self copy(*this);
        node_ = node_->next;
        return copy;
    }

    bool operator==(const Self&) const = default;

    BaseNode* node_;
};

}  // namespace detail
}  // namespace bmb
