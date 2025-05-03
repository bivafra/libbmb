# 🌟 libbmb
**libbmb** is a header only DSA library written using modern C++. It implements significant STL functionality and extends it.
The library's containers are *AllocatorAware* with *strong exception guarantee*.
The **key features** of this realization are **code readability**, **rich documentation**, and modern view to the **code design**.<br/>

All documentation is available **[here](https://bivafra.github.io/libbmb/)**.

## 🎯 Technology stack
* **Compiling** (*must support c++23*): [clang](https://clang.llvm.org/) and [gcc](https://gcc.gnu.org/)
* **Build system**: [cmake](https://cmake.org/) >=3.23 
* **Testing**: [gtest](https://github.com/google/googletest) 

## 🤔 Motivation
Why do we need one more DSA library while there'are STL, Boost, Abseil and many other great solutions?
There are several reasons for that:
1. **My personal interest.** 
    * I love to understand things deeply, and STL is one of these things. 
    In fact, implementing STL and DSA in general is much more challenging than one might imagine, 
    given sometimes tough ISO C++ requirements(e.g. exception safety, iterators time complexity, type erasure).
2. **Newbie friendly code.** 
    * When I started reading GCC libstdc++ sources, it was awful - infinite underscores, 
    strange code alignment, lack of inline comments, etc. While **libbmb** explains many moments, 
    understanding of the ideas how algorithms and containers work is required.

## 🛠️ Building
1. Clone the repo:
    ```shell
     git clone https://github.com/bivafra/libbmb.git
    ```
2. 
    1. If you're using the library as a dependency, add this to your `CMakeLists.txt`:
        ```shell
        add_subdirectory(libbmb)
        target_link_libraries(*your_target* PRIVATE libbmb)
        ```
        **libbmb** uses cmake's *interface*, so when linking you'll automatically get include directories.
    2. If you want to develop the library, you must write unit tests for your code. <br/> 
       To build tests you should install [valgrind](https://valgrind.org/) for memory checks, add the test files into `CMakeLists.txt`, and 
       run:
        ```shell
        make test
        ```
        This will automatically install `gtest` and run tests with `valgrind` checks. <br/><br/>
        The default compiler is `clang`. To build with `gcc` run:
        ```shell
        make test_gcc
        ```
        Note: if you use clangd as a language server, adjust the include path in the `.clangd` config file.
## ⚙️ Features
Following sections contain details about implemented and planned functionality, C++ standard compatibility, 
reasons why there's a deviation from the standard, and some other useful notes.

<!-- <details> -->
<!-- <summary><strong></strong></summary> -->
<!---->
<!-- </details> -->

<details>
<summary><strong>General</strong></summary>

1. Declaration and definition of templates.

    Since C++ obligates to write boilerplate template types like:
    ```c++
    template <typename T>
    class A {
    public:
        void foo();
    };

    // definition either later or in the included `.tpp` file
    template <typename T>
    void A<T>::foo() {...}
    ```
    it is painful to separatly declare and define template class methods. Boilerplate 
    can be avoided using strange looking macroses like:
    ```c++
    // Start of the `.tpp` file
    #define A_METHOD(return_type)\
    template <typename T>\
    return_type A<T>::

    // some code ...

    A_METHOD(void) foo() {...}

    // End of the file
    #undef A_METHOD
    ```
    The desicion was made to define methods inside a class. Documentation and 
    definition at the same place. If you don't like this - every modern IDE supports 
    code wrapping, so collapse all definitions by default.

</details>

<details>
<summary><strong>Allocator</strong></summary>

[source](include/utils/allocator.h)

- [x] PrimitiveAllocator
- [x] AllocatorTraits
- [ ] StackAllocator
- [ ] PoolAllocator
- [ ] polymorphic_allocator
- [ ] scoped_allocator_adaptor

There are changes in the allocator's design.
`std::allocator` analog here is `PrimitiveAllocator`. 
**It isn't a template**. The template parameter in class only creates pain 
and *rebind* semantics. Instead, every allocator's function marked as a template. 
Thus, each allocator must define at least `allocate` method as a template(other methods can 
use template type deduction, so may be non-templated).
* Pros:
    * There is no need for rebind at all, since the type will be either provided by
      the client or deduced by the compiler.
* Cons:
    * `allocate` method must be a template.
    * Due to the C++ rules, `allocate` call will look weird:
        ```c++
        using AllocTraits = bmb::AllocatorTraits<SomeAllocator>;
        SomeAllocator alloc;

        auto ptr = AllocTraits::template allocate<T>(alloc, 10);
        
        ```
        Meanwhile other methods usage the same as STL ones.

Also the allocator's copy and move semantics changed:<br/>
STL differentiates allocators copying/moving as value type(like string, vector and any other) and 
allocators copying/moving as container's fields. Therefore every container must consult a allocator 
what to do while copying/moving. **libbmb** preserve only the second semantics, so now each allocator **must** define 
copy/move c-tors/assignment operators with appropriate semantics. `propogate_on_container_...` and `select_on_container_copy_construction` are 
not used.
* Pros:
    * Easier containers implementation and support.
* Cons:
    * Less flexibility for allocators.  
    
</details>


<details>
<summary><strong>Move semantics</strong></summary>

[source](include/utils/move.h)
- [x] swap
    * You're free to overload it, but you should understand the consequences.
- [x] move
- [x] forward
- [ ] forward_like
    * Useful for *deducing this* c++23 

</details>


<details>
<summary><strong>Iterators</strong></summary>

[source](include/utils/iterators.h)

~~iterator_traits are not implemented, since there's no real need for them - 
every iterator still must define category, value_type, etc. However traits support 
raw pointer and have rules for defining missing typedefs.~~ There is real need for traits for raw pointers, otherwise
this is not possible use general algorithms(like std::copy) for pointers. Now, as in STL,
every Iter::*some type* must be IteratorTraits<Iter>::*some type*.

 
- [x] Iterator category tags
- [x] Iterator category concepts
- [x] IteratorTraits
- [ ] advance
- [ ] distance
- [ ] next
- [ ] prev
- [ ] Iterator adaptors:
    - [ ] reverse_iterator
    - [ ] move_iterator
    - [ ] back_inserter
    - [ ] front_inderter
    - [ ] inserter
- [ ] Stream iterator:
    - [ ] istream_iterator
    - [ ] ostream_iterator

</details>

<details>
<summary><strong>Vector</strong></summary>

sources: [1](include/containers/vector.h), [2](include/details/vector_iterator.h)<br/>
tests: [1](tests/vector)

This is a classic implementation of STL vector. It suports all original vector
methods except `shrink_to_fit` and `insert`. They will be added later.
The major difference from the STL one is an exception safety.
* For methods like `pushBack` and `reserve` conditional safety is provided:

    For the reallocation the move constructor is always chosen. If it throws,
    Vector destroyes all moved elements, leaving the original array with
    some empty elements and elements, that weren't moved. There is the ***basic guarantee***.
    Otherwise the ***strong guarantee***.<br/>
    Meanwhile, STL's version will choose move c-tor only if it is marked as noexcept.

Currently, there is a problem: the provided template type must not delete move c-tor
explicitly, since forwarding and moving will stop compile(`reserve`, `pushBack`, `emplaceBack`).
This can be fixed by concepts or SFINAE. Highly likely this will not be fixed due to the redundancy. 
Modern code must(at least from the author's point of view) either define both copy and move c-tor(may 
just call copy one if it isn't able to move) or doesn't define them at all.
</details>


<details>
<summary><strong>Type traits</strong></summary>

[source](include/utils/type_traits.h)

Some iterator traits will not be implemented, since c++20 concepts 
may almost completely replace them.
Now implemented the most important traits. Support will be extended with the growth of the codebase.

</details>


<details>
<summary><strong>Concepts</strong></summary>

[source](include/utils/concepts.h)

Now implemented the most important concepts. Support will be extended with the growth of the codebase.
</details>
