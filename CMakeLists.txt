cmake_minimum_required(VERSION 3.20)
project(trading_engine)

set(CMAKE_CXX_STANDARD 20)

add_executable(trading_engine
        src/main.cpp
        src/OrderBook.cpp
        src/MatchingEngine.cpp
        src/ThreadPool.cpp
)
target_include_directories(trading_engine PRIVATE src)

add_executable(tests
        src/tests.cpp
        src/OrderBook.cpp
        src/MatchingEngine.cpp
        src/ThreadPool.cpp
)
target_include_directories(tests PRIVATE src)
