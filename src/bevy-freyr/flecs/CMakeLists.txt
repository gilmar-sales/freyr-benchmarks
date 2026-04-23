add_executable(flecs_perf src/main.cpp)
target_link_libraries(flecs_perf PUBLIC flecs::flecs_static perfetto)

target_compile_definitions(flecs_perf PUBLIC PERFETTO_ENABLE)
target_compile_features(flecs_perf PUBLIC cxx_std_23)
