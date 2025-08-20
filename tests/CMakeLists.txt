cmake_minimum_required(VERSION 3.14)

# Download and setup Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Test files
add_executable(market_data_tests
    test_order_book.cpp
    test_market_data_engine.cpp
    test_lock_free_queue.cpp
    test_performance_stats.cpp
)

target_link_libraries(market_data_tests 
    market_data_engine 
    gtest 
    gtest_main
    Threads::Threads
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(market_data_tests)