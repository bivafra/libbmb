#pragma once
/**
 * @file concepts
 * @authors bivafra
 */

#include "utils/type_traits.h"

namespace bmb {

namespace detail {

template <typename T, typename U>
concept SameHelper = is_same_v<T, U>;

}  // namespace detail

// SameHelper is needed for correct subsuming.
// This is required for all commutative relations.

/// SameAs
template <typename T, typename U>
concept SameAs = detail::SameHelper<T, U> && detail::SameHelper<U, T>;

/// DefaultConstructible
template <typename T>
concept DefaultConstructible = requires { T(); };

/// ConstructibleFrom
template <typename T, typename... Args>
concept ConstructibleFrom = requires { T(declval<Args>()...); };

/// NothrowConstructibleFrom
template <typename T, typename... Args>
concept NothrowConstructibleFrom = requires {
    // There is no subsuming with ConstructibleFrom
    { T(declval<Args>()...) } noexcept;
};

/// MoveConstructible
template <typename T>
concept MoveConstructible = ConstructibleFrom<T, T&&>;

/// NothrowMoveConstructible
template <typename T>
concept NothrowMoveConstructible = NothrowConstructibleFrom<T, T&&>;

/// MoveAssignable
template <typename T>
concept MoveAssignable = requires(T a) { a = declval<T &&>(); };

/// NothrowMoveAssignable
template <typename T>
concept NothrowMoveAssignable = requires(T a) {
    { a = declval<T &&>() } noexcept;
};

/// CopyConstructible
template <typename T>
concept CopyConstructible = ConstructibleFrom<T, const T&>;

/// NothrowCopyConstructible
template <typename T>
concept NothrowCopyConstructible = NothrowConstructibleFrom<T, const T&>;

/// CopyAssignable
template <typename T>
concept CopyAssignable = requires(T a, T b) { a = b; };

/// NothrowCopyAssignable
template <typename T>
concept NothrowCopyAssignable = requires(T a, T b) {
    { a = b } noexcept;
};

}  // namespace bmb
