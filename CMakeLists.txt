cmake_minimum_required(VERSION 3.20)
project(trading_engine)
set(CMAKE_CXX_STANDARD 20)

add_executable(trading_engine
        src/main.cpp
        src/order_book.cpp
        src/matching_engine.cpp
        src/thread_pool.cpp
)
target_include_directories(trading_engine PRIVATE src)

add_executable(tests
        src/test.cpp
        src/order_book.cpp
        src/matching_engine.cpp
        src/thread_pool.cpp
)
target_include_directories(tests PRIVATE src)

add_executable(benchmark
        src/Benchmark.cpp
        src/order_book.cpp
        src/matching_engine.cpp
        src/thread_pool.cpp
)
target_include_directories(benchmark PRIVATE src)