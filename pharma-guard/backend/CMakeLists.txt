# Alternative to the Makefile (works with Visual Studio, MinGW, Linux, macOS).
#
#   cd backend
#   cmake -S . -B build
#   cmake --build build --config Release
cmake_minimum_required(VERSION 3.10)
project(PharmaGuard CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(pharma_guard ${SOURCES})
target_include_directories(pharma_guard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)
target_link_libraries(pharma_guard PRIVATE Threads::Threads)

if(WIN32)
  target_link_libraries(pharma_guard PRIVATE ws2_32)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(pharma_guard PRIVATE -Wall -Wextra -pedantic)
endif()
