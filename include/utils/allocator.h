#pragma once
/**
 * @file allocator.h
 * File provides allocator traits
 * @authors bivafra
 */

#include <cstddef>
#include <cstdio>
#include <new>

#include "utils/move.h"

namespace bmb {

/**
 * @class PrimitiveAllocator
 * @brief Allocator-proxy for new/delete operators
 *
 * This allocator doesn't manage any memory but simply
 * calls new/delete
 */
class PrimitiveAllocator {
   public:
    /**
     * @brief Allocates raw memory for n objects of type  T
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
template <typename Alloc>
class AllocatorTraits {
   public:
    using allocator_type = Alloc;

    /**
     * @brief Forwards given arguments to allocator's construct method
     * if present. Otherwise uses placement new
     *
     * @param alloc Allocator to use in construction
     * @param ptr Pointer to raw memory where to construct
     * @param args... Arguments to pass in constructor
     *
     * @throws Same as alloc.construct(...) if present. Otherwise
     * same specification as new
     */
    template <typename T, typename... Args>
    static void construct(Alloc& alloc, T* ptr, Args&&... args) {
        // Use allocator if can
        if constexpr (requires {
                          alloc.construct(ptr, forward<Args>(args)...);
                      }) {
            alloc.construct(ptr, forward<Args>(args)...);
        } else {
            new (ptr) T(forward<Args>(args)...);
        }
    }

    /**
     * @brief Destroys object throgh alloctor if destroy method is present.
     * Otherwise calls ~T()
     *
     * @param alloc Allocator to use in destruction
     * @param ptr Pointer to object to destroy
     *
     * @throws If allocator has method destroy - throws the same.
     * Otherwise doesn't throw(in assumption object destructor doesn't throw)
     */
    template <typename T>
    static void destroy(Alloc& alloc, T* ptr) {
        // Use allocator if can
        if constexpr (requires { alloc.destroy(ptr); }) {
            alloc.destroy(ptr);
        } else ptr->~T();
    }

    /**
     * @brief Allocates given number of objects using allocator
     *
     * @param alloc Allocator to use for allocation
     * @param n Number of objects to allocate
     *
     * @return Pointer to the allocated memory
     *
     * @throws Same as alloc.allocate(...)
     */
    template <typename T>
    static T* allocate(Alloc& alloc, size_t n) {
        return alloc.template allocate<T>(n);
    }

    /**
     * @brief Deallocates memory under ptr through allocator
     *
     * @param alloc Allocator to use in deallocation
     * @param ptr Pointer to the previously allocated memory
     * @param n Number of objects to deallocate
     *
     * @throws Same specification as alloc.deallocate()
     */
    template <typename T>
    static void deallocate(Alloc& alloc, T* ptr, size_t n) {
        alloc.deallocate(ptr, n);
    }
};

}  // namespace bmb
