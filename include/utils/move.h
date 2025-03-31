/**
 * @file move.h
 * This file provides move semantics features.
 * @authors bivafra
 */

#include "type_traits.h"

namespace bmb {

/**
 * @brief Convert a value to xvalue
 * @tparam value The object to cast to xvalue
 * @return The given parameter casted to rvalue reference
 */
template <typename T>
[[nodiscard("Move should be used to initialize other object")]]
constexpr remove_ref_t<T>&& move(T&& value) noexcept {
    // Universal reference as a parameter allows to move already rvalue
    // expressions. Therefore returning type should remove reference from the
    // deduced type(see reference collapsing rules)
    return static_cast<remove_ref_t<T>&&>(value);
}

/**
 * @brief Forward a lvalue
 * @tparam value The object to forward
 * @return Given value with proper expressions category
 */
template <typename T>
[[nodiscard("Forward should be used to initialize other object")]]
constexpr T&& forward(remove_ref_t<T>& value) noexcept {
    return static_cast<T&&>(value);
}

template <typename T>
    requires(!is_lvalue_ref_v<T>)
[[nodiscard("Forward should be used to initialize other object")]]
constexpr T&& forward(remove_ref_t<T>&& value) noexcept {
    return static_cast<T&&>(value);
}

}  // namespace bmb
