add_executable(
    compiler

    main.cpp
)

custom_enable_cpp23(compiler)
custom_enable_sanitizers(compiler)
custom_enable_strict_warnings(compiler)
custom_enable_clang_tidy(compiler)
custom_enable_coverage(compiler)

target_link_libraries(compiler PRIVATE libparse liblex)
