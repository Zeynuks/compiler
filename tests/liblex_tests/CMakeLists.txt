add_executable(
    liblex_tests

    LexerTest.cpp
    SourceFileTest.cpp
    TextScannerTest.cpp
    TokenTypeTest.cpp
)

custom_enable_cpp23(liblex_tests)
custom_enable_sanitizers(liblex_tests)
custom_enable_strict_warnings(liblex_tests)
custom_enable_clang_tidy(liblex_tests)
custom_enable_coverage(liblex_tests)

target_link_libraries(liblex_tests PRIVATE liblex Catch2::Catch2WithMain)

# Регистрируем тесты на Catch2 в CTest.
include(CTest)
include(Catch)
catch_discover_tests(liblex_tests)
