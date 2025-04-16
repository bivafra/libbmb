init_test_build:
	mkdir -p build
	cmake -B build/ -D BUILD_TESTS=ON -D CMAKE_CXX_COMPILER=clang++

init_no_test_build:
	mkdir -p build
	cmake -B build/ -D BUILD_TESTS=OFF -D CMAKE_CXX_COMPILER=clang++

test: init_test_build
	cmake --build build/
	./build/libbmb_tests

init_test_build_gcc:
	mkdir -p build
	cmake -B build/ -D BUILD_TESTS=ON -D CMAKE_CXX_COMPILER=g++

test_gcc: init_test_build_gcc
	cmake --build build/
	./build/libbmb_tests

clean:
	rm -rf build
