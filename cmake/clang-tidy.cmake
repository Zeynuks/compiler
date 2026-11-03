# clang-tidy: сначала в PATH, потом в каталогах LLVM
find_program(
    CLANG_TIDY_EXECUTABLE
    NAMES clang-tidy
    PATHS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin
)

if(NOT CLANG_TIDY_EXECUTABLE)
  message(WARNING "clang-tidy not found: static analysis is disabled")
endif()

# Функция custom_enable_clang_tidy включает статический анализатор clang-tidy для выбранной цели
function(custom_enable_clang_tidy TARGET)
  if(CLANG_TIDY_EXECUTABLE)
    message(STATUS "Enabling clang-tidy for target: ${TARGET}")
    set_target_properties(${TARGET} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}"
        )
  endif()
endfunction()
