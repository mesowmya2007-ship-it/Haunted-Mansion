CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic

all: build/ravenwood

build/ravenwood: src/main.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) src/main.cpp -o build/ravenwood

run: build/ravenwood
	./build/ravenwood

clean:
	rm -rf build

.PHONY: all run clean
