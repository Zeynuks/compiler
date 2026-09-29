cmake_minimum_required(VERSION 4.3)
project(compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(extern/googletest)
add_subdirectory(extern/gmock)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif ()

add_subdirectory(lab-1)
