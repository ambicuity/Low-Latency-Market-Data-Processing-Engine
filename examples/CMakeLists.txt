cmake_minimum_required(VERSION 3.14)

# Example applications
add_executable(market_data_example
    market_data_example.cpp
)

add_executable(performance_test
    performance_test.cpp
)

target_link_libraries(market_data_example 
    market_data_engine 
    Threads::Threads
)

target_link_libraries(performance_test
    market_data_engine 
    Threads::Threads
)