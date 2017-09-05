cmake_minimum_required(VERSION 2.8)

project(mini_json)

add_subdirectory(json)
add_subdirectory(map)
add_subdirectory(memory)

add_executable(m_test test.c)

target_link_libraries(m_test Json)
target_link_libraries(m_test Map)
target_link_libraries(m_test Memory)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
