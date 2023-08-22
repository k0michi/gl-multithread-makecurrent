cmake_minimum_required(VERSION 3.12)
project(gl_multithread_makecurrent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(main main.cpp)

# SDL
add_subdirectory(third_party/SDL)
target_link_libraries(main SDL2-static)
target_include_directories(main PRIVATE third_party/SDL/include)

# glad
target_sources(main PRIVATE third_party/glad/src/gl.c)
target_include_directories(main PRIVATE third_party/glad/include)