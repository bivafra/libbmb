#pragma once

/**
 * @file compare.h
 * File provides comparing functions/functors
 * @authors bivafra
 */

namespace bmb {

/// compare_three_way
struct compare_three_way {
    template <typename T, typename U>
    auto operator()(const T& x, const U& y) const {
        return x <=> y;
    }
};

/// equal_to
struct equal_to {
    template <typename T, typename U>
    bool operator()(const T& x, const U& y) const {
        return x == y;
    }
};

/// not_equal_to
struct not_equal_to {
    template <typename T, typename U>
    bool operator()(const T& x, const U& y) const {
        return x != y;
    }
};

/// less
struct less {
    template <typename T, typename U>
    bool operator()(const T& x, const U& y) const {
        return x < y;
    }
};

/// less_equal
struct less_equal {
    template <typename T, typename U>
    bool operator()(const T& x, const U& y) const {
        return x <= y;
    }
};

/// greater
struct greater {
    template <typename T, typename U>
    bool operator()(const T& x, const U& y) const {
        return x > y;
    }
};

/// greater_equal
struct greater_equal {
    template <typename T, typename U>
    bool operator()(const T& x, const U& y) const {
        return x >= y;
    }
};

}  // namespace bmb
