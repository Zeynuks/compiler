add_library(
    libparse

    Parser.h
    Parser.cpp
)

target_include_directories(
    libparse
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

custom_enable_cpp23(libparse)
custom_enable_sanitizers(libparse)
custom_enable_strict_warnings(libparse)
custom_enable_clang_tidy(libparse)
custom_enable_coverage(libparse)

target_link_libraries(libparse PUBLIC libvm liblex)
