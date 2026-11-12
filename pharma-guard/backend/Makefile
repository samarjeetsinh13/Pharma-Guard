# Pharma Guard backend - build with:  make        (then run:  make run)
#
# Needs: a C++17 compiler (g++ 8 or newer / clang++) and GNU make.
# On Windows use MSYS2 (UCRT64) - see the README - or use CMake / the .bat scripts.

CXX      ?= g++
CXXFLAGS += -std=c++17 -O2 -Wall -Wextra -pedantic -pthread -Iinclude
LDFLAGS  += -pthread

TARGET := build/pharma_guard
ifeq ($(OS),Windows_NT)
  TARGET  := build/pharma_guard.exe
  LDLIBS  += -lws2_32
endif

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,build/%.o,$(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

build/%.o: src/%.cpp $(wildcard include/*.h) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

# Run from the project root so the program finds the data/ folder.
run: all
	cd .. && backend/$(TARGET)

demo: all
	cd .. && backend/$(TARGET) --demo

clean:
	rm -rf build

.PHONY: all run demo clean
