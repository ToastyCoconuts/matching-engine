cmake_minimum_required(VERSION 3.16)
project(matching_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main executable
add_executable(engine
    src/main.cpp
)

target_compile_options(engine PRIVATE
    -O2
    -march=native
    -Wall
    -Wextra
    -pthread
)

add_executable(tests
    tests/test_order_book.cpp
)

target_compile_options(tests PRIVATE
    -O2
    -march=native
    -Wall
    -Wextra
    -pthread
)


target_link_libraries(engine PRIVATE pthread)

# Enable thread sanitizer for debug builds
# toggle on when testing multithreaded code
# set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=thread -g")