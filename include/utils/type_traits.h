#pragma once
/**
 * @file type_traits.h
 * @authors bivafra
 */

namespace bmb {

/// declval
template <typename T>
T&& declval() {
    static_assert(false, "declval isn't implemented to call it");
}

// This type forces non-deduced context.
// It works because compiler's not able to deduce "qualifed name"
// like "type_identity<MyType>::value".
//
// It useful for forcing a client to call a function with
// explicitly given template types.
// For more explanation see https://stackoverflow.com/a/25245676

/// type_identity
template <typename T>
struct type_identity {
    using type = T;
};

/// type_identity_t
template <typename T>
using type_identity_t = type_identity<T>::type;

/// integral_constant
template <auto val>
struct integral_constant {
    using value_type            = decltype(val);
    static constexpr auto value = val;
};

/// bool_constant
template <bool val>
using bool_constant = integral_constant<val>;

/// true_type
using true_type  = bool_constant<true>;
/// false_type
using false_type = bool_constant<false>;

/// Logical AND metafunction
template <typename... Types>
struct conjunction {
    static constexpr bool value = (Types::value && ...);
};

/// Logical AND metafunction
template <typename... Types>
constexpr bool conjunction_v = conjunction<Types...>::value;

/// Logical OR metafunction
template <typename... Types>
struct disjunction {
    static constexpr bool value = (Types::value || ...);
};

/// Logical OR metafunction
template <typename... Types>
constexpr bool disjunction_v = disjunction<Types...>::value;

/// is_same
template <typename T, typename U>
struct is_same : false_type {};

template <typename T>
struct is_same<T, T> : true_type {};

/// is_same_v
template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

/// is_lvalue_ref
template <typename T>
struct is_lvalue_ref : false_type {};

template <typename T>
struct is_lvalue_ref<T&> : true_type {};

/// is_lvalue_ref_v
template <typename T>
constexpr bool is_lvalue_ref_v = is_lvalue_ref<T>::value;

/// is_lvalue_ref
template <typename T>
struct is_rvalue_ref : false_type {};

template <typename T>
struct is_rvalue_ref<T&&> : true_type {};

/// is_rvalue_ref_v
template <typename T>
constexpr bool is_rvalue_ref_v = is_rvalue_ref<T>::value;

/// is_const
template <typename T>
struct is_const : false_type {};

template <typename T>
struct is_const<const T> : true_type {};

/// is_const_v
template <typename T>
constexpr bool is_const_v = is_const<T>::value;

namespace detail {

// Only classes and unions can have pointers to members, even if
// there is no such member in the class.
template <typename T>
true_type test_is_class(int T::*);

template <typename>
false_type test_is_class(...);

}  // namespace detail

// WARNING: this implementation of is_class detects unions as classes
// because union is the same as struct/class but fields size.
// Also it's impossible to differentiate class and union without
// using compiler internal information.

/// is_class
template <typename T>
struct is_class : decltype(detail::test_is_class<T>(nullptr)) {};

/// is_class_v
template <typename T>
constexpr bool is_class_v = is_class<T>::value;

/// is_pointer
template <typename T>
struct is_pointer : false_type {};

template <typename T>
struct is_pointer<T*> : true_type {};

/// is_pointer_v
template <typename T>
constexpr bool is_pointer_v = is_pointer<T>::value;

/// remove_const
template <typename T>
struct remove_const {
    using type = T;
};

template <typename T>
struct remove_const<const T> {
    using type = T;
};

/// remove_const_t
template <typename T>
using remove_const_t = remove_const<T>::type;

/// remove_ref
template <typename T>
struct remove_ref {
    using type = T;
};

template <typename T>
struct remove_ref<T&> {
    using type = T;
};

template <typename T>
struct remove_ref<T&&> {
    using type = T;
};

/// remove_ref_t
template <typename T>
using remove_ref_t = remove_ref<T>::type;

/// conditional
template <bool Cond, typename IfTrue, typename IfFalse>
struct conditional {
    using type = IfTrue;
};

template <typename IfTrue, typename IfFalse>
struct conditional<false, IfTrue, IfFalse> {
    using type = IfFalse;
};

/// conditional_t
template <bool Cond, typename IfTrue, typename IfFalse>
using conditional_t = conditional<Cond, IfTrue, IfFalse>::type;

/// enable_if
template <bool Cond, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

/// enable_if_t
template <bool Cond, typename T = void>
using enable_if_t = enable_if<Cond, T>::type;

namespace detail {

// NOTE: here we need another one level of redirection,
// because without it SFINAE won't work: test_ptr_cast(const B*) is
// a valid declaration, thefore, the compiler choose it over others overloads,
// and only then it checks avalability/accessibility, which causes CE in case of
// private/disambiguous inheritance. However the 'decltype' below enables SFINAE.

template <typename B>
true_type test_ptr_cast(const B*);
template <typename>
false_type test_ptr_cast(const void*);

template <typename B, typename D>
auto test_is_base_of(int) -> decltype(test_ptr_cast<B>(static_cast<D*>(nullptr)));
template <typename, typename>
auto test_is_base_of(...) -> true_type;  // private/disambiguous base

}  // namespace detail

/// is_base_of
template <typename Base, typename Derived>
struct is_base_of : conjunction<
                        is_class<Base>,
                        is_class<Derived>,
                        decltype(detail::test_is_base_of<Base, Derived>(0))> {};

/// is_base_of_v
template <typename Base, typename Derived>
constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;

}  // namespace bmb
