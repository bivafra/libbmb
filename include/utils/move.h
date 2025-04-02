#pragma once
/**
 * @file move.h
 * This file provides move semantics features.
 * @authors bivafra
 */

#include "utils/concepts.h"
#include "utils/type_traits.h"

namespace bmb {

/**
 * @brief Swap values
 *
 * @param x Value to swap
 * @param y Value to swap
 *
 * @throws The same exception safety as move c-tor and
 * move assignmet operator
 * of T
 */
template <typename T>
void swap(T& x, T& y) noexcept(MoveConstructible<T> && MoveAssignable<T>) {
    T tmp = move(x);

    x = move(y);
    y = move(tmp);
}

/**
 * @brief Convert a value to xvalue
 * @param value The object to cast to xvalue
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
 * @param value The object to forward
 * @return Given value with proper expressions category
 */
template <typename T>
[[nodiscard("Forward should be used to initialize other object")]]
constexpr T&& forward(remove_ref_t<T>& value) noexcept {
    return static_cast<T&&>(value);
}

/**
 * @brief Forward a rvalue
 * @param value The object to forward
 * @return Given value with proper expressions category
 */
template <typename T>
    requires(!is_lvalue_ref_v<T>)
[[nodiscard("Forward should be used to initialize other object")]]
constexpr T&& forward(remove_ref_t<T>&& value) noexcept {
    return static_cast<T&&>(value);
}

}  // namespace bmb
