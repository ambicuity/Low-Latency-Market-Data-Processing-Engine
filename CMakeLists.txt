cmake_minimum_required(VERSION 3.14)
project(MarketDataEngine CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags for optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra -Wpedantic")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find packages
find_package(Threads REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Create library
add_library(market_data_engine STATIC ${SOURCES} ${HEADERS})
target_link_libraries(market_data_engine Threads::Threads)

# Examples
add_subdirectory(examples)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Benchmarks
add_subdirectory(benchmarks)