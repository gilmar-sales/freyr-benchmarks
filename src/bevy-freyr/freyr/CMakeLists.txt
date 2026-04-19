
add_executable(freyr_parallel src/main.cpp)

target_link_libraries(freyr_parallel PUBLIC freyr::freyr)