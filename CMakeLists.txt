cmake_minimum_required(VERSION 3.23) # should support FILE_SET
project(libbmb)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILER clang++)

set(RELEASE_OPTIONS "-Wall" "-O2")
set(DEBUG_OPTIONS "-Wall -g")

set(
  SRC_LIST 
)  

add_library(libbmb ${SRC_LIST})

target_include_directories(libbmb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# find_package(GTest REQUIRED)
# include_directories(${GTEST_INCLUDE_DIRS})

# add_executable(${TEST_TARGET} ${TEST_SRC})

# target_link_libraries(${TEST_TARGET} GTest::gtest_main)
# target_link_libraries(${TEST_TARGET} MyLibrary)
