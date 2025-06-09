init_test_build:
	mkdir -p build
	cmake -B build/ -D BUILD_TESTS=ON -D CMAKE_CXX_COMPILER=clang++

init_test_build_gcc:
	mkdir -p build
	cmake -B build/ -D BUILD_TESTS=ON -D CMAKE_CXX_COMPILER=g++

init_no_test_build:
	mkdir -p build
	cmake -B build/ -D BUILD_TESTS=OFF -D CMAKE_CXX_COMPILER=clang++

build_tests:
	cmake --build build/

test: init_test_build build_tests
	valgrind --leak-check=yes ./build/libbmb_tests

test_gcc: init_test_build_gcc build_tests
	valgrind --leak-check=yes ./build/libbmb_tests

clean:
	rm -rf build
