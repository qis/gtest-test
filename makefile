MAKEFLAGS += --no-print-directory

CC	:= clang
CXX	:= clang++
CMAKE	:= CC=$(CC) CXX=$(CXX) cmake -GNinja
PROJECT	!= grep "^project" CMakeLists.txt | cut -c9- | cut -d " " -f1
SOURCES	!= find src -type f

all: debug

run: debug
	@build/llvm/debug/$(PROJECT) | sed "s|^../../..|$(PWD)|"

test: debug
	@cd build/llvm/debug && ctest --build-run-dir $(PWD)

debug: build/llvm/debug/CMakeCache.txt $(SOURCES)
	@cmake --build build/llvm/debug

build/llvm/debug/CMakeCache.txt: CMakeLists.txt res/config.h.in build/llvm/debug
	@cd build/llvm/debug && $(CMAKE) -DCMAKE_BUILD_TYPE=Debug $(PWD)

build/llvm/debug:
	@mkdir -p build/llvm/debug

clean:
	rm -rf build/llvm

.PHONY: all run test debug clean
