#pragma once
/**
 * @file allocator.h
 * File provides allocator traits
 * @authors bivafra
 */

#include <cstddef>

/**
 * @class PrimitiveAllocator
 * @brief Allocator-proxy for new/delete operators
 *
 * This allocator doesn't manage any memory but simply
 * calls new/delete
 */
class PrimitiveAllocator {
    /**
     * @brief Allocate raw memory for n objects of type  T
     *
     * @param n Number of objects to allocate
     * @return Pointer to the allocated memory
     *
     * @throws Same specification as for new operator
     */
    template <typename T>
    T* allocate(size_t n) {
        return static_cast<T*>(operator new(n * sizeof(T)));
    }

    /**
     * @brief Deallocates memory under ptr
     *
     * Takes dummy argument which semantically means
     * "how many objects were allocated at that memory".
     * This is done for unifying interface with any allocator that
     * might need this number.
     *
     * @param ptr Pointer to the previously allocated memory
     * @param _ Dummy argument
     * @return Description of the returned value
     *
     * @throws noexcept
     */
    template <typename T>
    void deallocate(T* ptr, size_t) noexcept {
        operator delete(ptr);
    }
};

/**
 * @class AllocatorTraits
 * @brief Unifined interface for allocators.
 *
 * All callers to any allocator should do it throw
 * this class-proxy. It predefines some default methods
 * in case a allocator dosn't do it. Otherwise it calls
 * the allocator's method.
 */
// template <typename T>
// class AllocatorTraits {};
