include_directories(../../src/)

add_executable(
    acceptance_tests

    AcceptanceRunner.cpp
)

custom_enable_cpp23(acceptance_tests)
custom_enable_sanitizers(acceptance_tests)
custom_enable_strict_warnings(acceptance_tests)
custom_enable_clang_tidy(acceptance_tests)
custom_enable_coverage(acceptance_tests)

target_link_libraries(acceptance_tests PRIVATE
    Catch2::Catch2WithMain
    liblex
    libvm
    libparse
)

add_test(NAME "Acceptance tests from files" COMMAND acceptance_tests)
set_tests_properties("Acceptance tests from files" PROPERTIES LABELS acceptance)
