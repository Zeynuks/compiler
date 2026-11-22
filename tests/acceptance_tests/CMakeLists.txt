add_executable(acceptance_tests
        AcceptanceRunner.cpp
)

target_link_libraries(acceptance_tests PRIVATE
        Catch2::Catch2WithMain
        liblex
        libvm
        libparse
)

# Включаем санитайзеры и покрытие, как в основном проекте
target_enable_sanitizers(acceptance_tests)
target_enable_coverage(acceptance_tests)

include(Catch)
catch_discover_tests(acceptance_tests)