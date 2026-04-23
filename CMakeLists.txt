cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TBB_TEST OFF)
project(FreyrBenchmarks)

option(BENCHMARK_ENABLE_TESTING "Disable tests" OFF)

include(FetchContent)

FetchContent_Declare(
        google-benchmark
        GIT_REPOSITORY "https://github.com/google/benchmark.git"
        GIT_TAG "v1.9.5"
)
FetchContent_MakeAvailable(google-benchmark)

FetchContent_Declare(
        glm
        GIT_REPOSITORY "https://github.com/g-truc/glm.git"
        GIT_TAG "1.0.3"
)
FetchContent_MakeAvailable(glm)

option(FREYR_PROFILING "Enable profiling" ON)
FetchContent_Declare(
        freyr
        GIT_REPOSITORY "https://github.com/gilmar-sales/Freyr.git"
        GIT_TAG "v0.26.0"
)
FetchContent_MakeAvailable(freyr)

FetchContent_Declare(
        entt
        GIT_REPOSITORY "https://github.com/skypjack/entt.git"
        GIT_TAG "v3.16.0"
)
FetchContent_MakeAvailable(entt)


FetchContent_Declare(
        tbb
        GIT_REPOSITORY "https://github.com/oneapi-src/oneTBB.git"
        GIT_TAG "v2022.3.0"
)
FetchContent_MakeAvailable(tbb)

FetchContent_Declare(
        flecs
        GIT_REPOSITORY "https://github.com/SanderMertens/flecs.git"
        GIT_TAG "v4.1.5"
)
FetchContent_MakeAvailable(flecs)
target_compile_definitions(flecs PUBLIC FLECS_PERF_TRACE)

# add_subdirectory(vendor/perfetto)

if(CMAKE_BUILD_TYPE STREQUAL "Release")

  # ----- LTO / IPO -----
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR LANGUAGES C CXX)

  if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "Release: IPO / LTO enabled")
  else()
    message(WARNING "Release: IPO not supported: ${IPO_ERROR}")
  endif()

  # ----- march=native (GCC/Clang) / arch=native (MSVC) -----
  if(MSVC)
    add_compile_options(/arch:native)
  else()
    add_compile_options(-march=native)
  endif()

  # ----- Additional Release Optimizations -----
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -ffast-math -funroll-loops)
  elseif(MSVC)
    add_compile_options(/O2 /fp:fast)
  endif()

endif()

add_executable(conjunto_trabalho src/conjunto_trabalho.cpp)
add_executable(polimorfismo src/polimorfismo.cpp)
add_executable(update_method_dod src/update_method_dod.cpp)
add_executable(dispersao_memoria src/dispersao_memoria.cpp)
add_executable(dispersao_memoria_execucao src/dispersao_memoria_execucao.cpp)
add_executable(layout_memoria src/layout_memoria.cpp)
add_executable(divisao_carga_trabalho src/divisao_carga_trabalho.cpp)
add_executable(ecs_iteracao_sequencial src/ecs_iteracao_sequencial.cpp)
add_executable(ecs_iteracao_assincrona src/ecs_iteracao_assincrona.cpp)
add_executable(ecs_iteracao_assincrona_profile src/ecs_iteracao_assincrona_profile.cpp)
add_executable(ecs_particionamento_espacial src/ecs_particionamento_espacial.cpp src/Containers/Octree.cpp)
add_executable(ecs_particionamento_espacial_profile src/ecs_particionamento_espacial_profile.cpp src/Containers/Octree.cpp)

target_compile_features(divisao_carga_trabalho PUBLIC cxx_std_17)
target_compile_features(conjunto_trabalho PUBLIC cxx_std_20)

target_link_libraries(conjunto_trabalho benchmark::benchmark glm)
target_link_libraries(polimorfismo benchmark::benchmark)
target_link_libraries(update_method_dod benchmark::benchmark)
target_link_libraries(dispersao_memoria_execucao benchmark::benchmark)
target_link_libraries(layout_memoria benchmark::benchmark)
target_link_libraries(divisao_carga_trabalho PUBLIC perfetto benchmark::benchmark)
target_link_libraries(ecs_iteracao_assincrona PUBLIC benchmark::benchmark freyr::freyr EnTT::EnTT TBB::tbb flecs::flecs_static)
target_link_libraries(ecs_iteracao_sequencial PUBLIC benchmark::benchmark freyr::freyr EnTT::EnTT flecs::flecs_static)
target_link_libraries(ecs_iteracao_assincrona_profile PUBLIC perfetto benchmark::benchmark freyr::freyr EnTT::EnTT)
target_link_libraries(ecs_particionamento_espacial PUBLIC benchmark::benchmark freyr::freyr EnTT::EnTT glm TBB::tbb flecs::flecs_static)
target_link_libraries(ecs_particionamento_espacial_profile PUBLIC perfetto benchmark::benchmark freyr::freyr EnTT::EnTT glm)

target_compile_options(ecs_iteracao_assincrona_profile PRIVATE
    -fno-inline
    -fno-inline-small-functions
    -fno-inline-functions-called-once
)

add_subdirectory(src/bevy-freyr/freyr)
add_subdirectory(src/bevy-freyr/entt_tbb)
add_subdirectory(src/bevy-freyr/flecs)
