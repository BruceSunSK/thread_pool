cmake_minimum_required(VERSION 3.0.2)
project(thread_pool)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

add_library(thread_pool SHARED src/thread_pool.cpp)

add_executable(main main.cpp)
target_link_libraries(main thread_pool)