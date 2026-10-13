add_library(
    liblex

    CharClass.h
    Lexer.h
    SourceFile.h
    SourceLocation.h
    TextScanner.h
    Token.h
    TokenType.h

    Lexer.cpp
    SourceFile.cpp
    TextScanner.cpp
    TokenType.cpp
)

target_include_directories(
    liblex
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

custom_enable_cpp23(liblex)
custom_enable_sanitizers(liblex)
custom_enable_strict_warnings(liblex)
custom_enable_clang_tidy(liblex)
custom_enable_coverage(liblex)
