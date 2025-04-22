# 🌟 libbmb
**libbmb** is a header only DSA library written using modern C++. It implements significant STL functionality and extends it.
The library's containers are *AllocatorAware* with *strong exception guarantee*.
The **key features** of this realization are **code readability**, **rich documentation**, and different view to the **design**.<br/>

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
    1. If you're using the library as a dependency, add this to your CMakeLists.txt:
        ```shell
        add_subdirectory(libbmb)
        target_link_libraries(*your_target* PRIVATE libbmb)
        ```
        **libbmb** uses cmake's *interface*, so when linking you'll automatically get include directories.
    2. If you're developing the library, the only options is to build the tests:
        ```shell
        make test
        ```
    Note: if you use clangd, adjust the include path in the .clangd config file.
## ⚙️ Features
Following sections contain details about implemented and planned functionality, C++ standard compatibility, 
reasons why there's a deviation from the standard, and some other useful notes.

<!-- <details> -->
<!-- <summary><strong></strong></summary> -->
<!---->
<!-- </details> -->

<details>
<summary><strong>Allocator</strong></summary>

[source](include/utils/allocator.h)

- [x] PrimitiveAllocator
- [x] AllocatorTraits
- [ ] StackAllocator
- [ ] PoolAllocator
- [ ] polymorphic_allocator
- [ ] scoped_allocator_adaptor

There are major changes in the allocator's design.
*std::allocator* analog here is *PrimitiveAllocator*. 
**It isn't a template**. The template parameter in class only creates pain 
and *rebid* semantics. Instead, every allocator's function marked as a template. 
Thus, each allocator must define all methods as templates.
* Pros:
    * There is no need for rebind at all, since the type will be provided with each call.
* Cons:
    * Every method must be a template.
    * Due to the c++ rules, *allocate* will look weird:
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
    * Much easier containers implementation and support.
* Cons:
    * Less flexibility for allocators.  
    
</details>


<details>
<summary><strong>Move semantics</strong></summary>

[source](include/utils/move.h)
- [x] swap
    * You're free to overload it
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

Work progress.

</details>


<details>
<summary><strong>Type traits</strong></summary>

[source](include/utils/type_traits.h)

Some iterator traits will not be implemented, since c++20 concepts 
may completely replace them.
Now implemented the most important traits. Support will be extended with the growth of the codebase.

</details>


<details>
<summary><strong>Concepts</strong></summary>

[source](include/utils/concepts.h)

Now implemented the most important concepts. Support will be extended with the growth of the codebase.
</details>



