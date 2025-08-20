cmake_minimum_required(VERSION 3.14)

# Download and setup Google Benchmark
include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.7.1
)
FetchContent_MakeAvailable(googlebenchmark)

# Benchmark executable
add_executable(market_data_benchmarks
    benchmark_main.cpp
    benchmark_order_book.cpp
    benchmark_lock_free_queue.cpp
    benchmark_engine.cpp
)

target_link_libraries(market_data_benchmarks 
    market_data_engine
    benchmark::benchmark
    Threads::Threads
)