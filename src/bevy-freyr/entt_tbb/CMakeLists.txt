add_executable(entt_tbb src/main.cpp)

target_link_libraries(entt_tbb PUBLIC EnTT::EnTT TBB::tbb freyr::freyr)
target_compile_definitions(entt_tbb PUBLIC PERFETTO_ENABLE)
