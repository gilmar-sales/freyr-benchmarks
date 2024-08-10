cmake_minimum_required(VERSION 3.28)

project(FreyrBenchmarks)

option(BENCHMARK_ENABLE_TESTING "asd" OFF)

add_subdirectory(vendor/google-benchmark)

add_executable(freyr-benchmarks src/main.cpp)

target_link_libraries(freyr-benchmarks benchmark::benchmark)

