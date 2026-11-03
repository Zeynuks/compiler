cmake_minimum_required(VERSION 3.28)

project(compiler LANGUAGES CXX)

# Стандарт C++23 нужен и сторонним зависимостям, собираемым вместе с проектом.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Предпочитаем установленный в системе Catch2, иначе скачиваем его.
find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
        SYSTEM
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

enable_testing()

# Подключаем CMake-модули данного проекта.
include(cmake/cpp23.cmake)
include(cmake/warnings.cmake)
include(cmake/sanitizers.cmake)
include(cmake/clang-tidy.cmake)
include(cmake/coverage.cmake)

# Экспортируем команды для использования с clangd и clang-tidy.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Подключаем каталоги модулей этого проекта.
add_subdirectory(src/liblex)

# Подключаем каталоги тестов этого проекта.
add_subdirectory(tests/liblex_tests)
