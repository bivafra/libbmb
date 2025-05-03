#pragma once
/**
 * @file types.h
 * File contains general types that useful for testing, like
 * no default c-tor, class that manages heap memory etc.
 * @authors bivafra
 */

#include <exception>

template <typename Type>
struct DefaultTraits {
    using Self = Type;
};

/**
 * @class ManagesHeapMemory
 * @brief Boilerplate class that makes memory allocatations.
 * Main purpose: control balance of operations(like no more than 1 destructor is called).
 */
class ManagesHeapMemory : private DefaultTraits<ManagesHeapMemory> {
public:
    ManagesHeapMemory()
        : ptr_(new int(1)) {}

    ManagesHeapMemory(int x)
        : ptr_(new int(x)) {}

    ManagesHeapMemory(const Self& other)
        : ptr_(new int(*other.ptr_)) {}

    ManagesHeapMemory(Self&& other)
        : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    Self& operator=(const Self& other) {
        if (this == &other) return *this;

        delete ptr_;
        ptr_ = new int(*other.ptr_);

        return *this;
    }

    Self& operator=(Self&& other) {
        if (this == &other) return *this;

        delete ptr_;
        ptr_ = other.ptr_;

        other.ptr_ = nullptr;

        return *this;
    }

    ~ManagesHeapMemory() { delete ptr_; }

    bool operator==(const Self& other) const {
        if (ptr_ == nullptr
            || other.ptr_ == nullptr) return ptr_ == other.ptr_;
        return *ptr_ == *other.ptr_;
    }

private:
    int* ptr_ = nullptr;
};

/// NoDefaultCtor
struct NoDefaultCtor {
public:
    NoDefaultCtor() = delete;

    NoDefaultCtor(NoDefaultCtor&&)                 = default;
    NoDefaultCtor(const NoDefaultCtor&)            = default;
    NoDefaultCtor& operator=(NoDefaultCtor&&)      = default;
    NoDefaultCtor& operator=(const NoDefaultCtor&) = default;
    ~NoDefaultCtor()                               = default;
};

// TODO: consider refactoring: rename to AllowNthChainCopies
// with according semantics changes

/**
 * @class ThrowOnNthChainCopy
 * @brief Class without move operations, that throws std::exception when
 * it is copied n-th times by chain.
 *
 * Useful for testing exception safety by simulating throws
 * at "random" time.
 *
 * Example:
 * ThrowOnNthChainCopy a(2);
 *
 * ThrowOnNthChainCopy b(a); // no exception
 * ThrowOnNthChainCopy c(a); // no exception
 * ThrowOnNthChainCopy d(a); // no exception
 *
 * ThrowOnNthChainCopy e(b); // throws
 * ThrowOnNthChainCopy f(b); // throws
 * ThrowOnNthChainCopy g(c); // throws
 *
 */
struct ThrowOnNthChainCopy {
public:
    ThrowOnNthChainCopy() = default;
    explicit ThrowOnNthChainCopy(int copy_num_to_throw, int payload = 0)
        : payload_(payload)
        , copies_allowed_(copy_num_to_throw - 1) {}

    // Implicitly deleted. Don't uncomment, it will break perfect forwarding.
    // ThrowOnCopy(ThrowOnCopy&&)            = delete;
    // ThrowOnCopy& operator=(ThrowOnCopy&&) = delete;

    ThrowOnNthChainCopy(const ThrowOnNthChainCopy& other) {
        if (other.copies_allowed_ == 0) throw std::exception();

        payload_        = other.payload_;
        copies_allowed_ = other.copies_allowed_ - 1;
    }

    ThrowOnNthChainCopy& operator=(const ThrowOnNthChainCopy& other) {
        if (this == &other) return *this;

        if (other.copies_allowed_ == 0) throw std::exception();

        payload_        = other.payload_;
        copies_allowed_ = other.copies_allowed_ - 1;
        return *this;
    }

    ~ThrowOnNthChainCopy() = default;

    bool operator==(const ThrowOnNthChainCopy& other) const { return payload_ == other.payload_; }

private:
    int payload_        = 0;
    int copies_allowed_ = 0;
};
