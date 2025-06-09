cmake_minimum_required(VERSION 3.23) # should support FILE_SET
project(libbmb LANGUAGES CXX)

set(LIB_NAME libbmb)
set(TEST_EXE libbmb_tests)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build tests for the library" OFF)

add_library(${LIB_NAME} INTERFACE)

target_include_directories(${LIB_NAME} INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(${LIB_NAME} INTERFACE cxx_std_23)

# Testing with google test
if(BUILD_TESTS)
    enable_testing()

    # Taken from the official gtest docs.
    # Here the version is fixed, to upgrade just change the version from the official repo.
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG        v1.17.0
    )
    FetchContent_MakeAvailable(googletest)

    # Test sources here
    add_executable(${TEST_EXE}
        tests/vector/vector_test.cpp
        tests/linked_list/linked_list_test.cpp
    )

    target_link_libraries(${TEST_EXE} PRIVATE
        GTest::gtest_main
        ${LIB_NAME}
    )

    target_include_directories(${TEST_EXE} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    include(GoogleTest)
    gtest_discover_tests(${TEST_EXE})
endif()
