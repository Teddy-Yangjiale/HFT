cmake_minimum_required(VERSION 3.20)

project(HFT
    VERSION 0.1.0
    DESCRIPTION "C++ high-frequency trading system MVP"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(hft_core
    src/book.cpp
    src/csv_market_data.cpp
    src/fixed_spread_market_maker.cpp
    src/market_event_journal.cpp
    src/order_manager.cpp
    src/risk_engine.cpp
    src/sequence_gap_detector.cpp
    src/simulated_exchange.cpp
)

target_include_directories(hft_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(hft_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_executable(hft_replay_app
    src/main.cpp
)

target_link_libraries(hft_replay_app PRIVATE hft_core)

add_executable(hft_benchmark_app
    benchmarks/replay_benchmark.cpp
)

target_link_libraries(hft_benchmark_app PRIVATE hft_core)

enable_testing()

add_executable(hft_tests
    tests/test_main.cpp
)

target_link_libraries(hft_tests PRIVATE hft_core)

add_test(NAME hft_tests COMMAND hft_tests)
set_tests_properties(hft_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
