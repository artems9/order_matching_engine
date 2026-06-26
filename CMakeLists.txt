cmake_minimum_required(VERSION 3.20)
project(trading_engine)
set(CMAKE_CXX_STANDARD 20)

# shared code — built once, linked into all three executables
add_library(engine_lib
        src/OrderBook.cpp
        src/MatchingEngine.cpp
        src/ThreadPool.cpp
)
target_include_directories(engine_lib PUBLIC include)

# executables
add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE engine_lib)

add_executable(tests src/tests.cpp)
target_link_libraries(tests PRIVATE engine_lib)

add_executable(benchmark src/benchmarks.cpp)
target_link_libraries(benchmark PRIVATE engine_lib)