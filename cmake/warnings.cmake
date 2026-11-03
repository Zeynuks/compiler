# Функция custom_enable_strict_warnings включает режим строгих предупреждений для выбранной цели
function(custom_enable_strict_warnings TARGET)
  message(STATUS "Enabling strict warnings for target: ${TARGET}")

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${TARGET} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            -Wundef
            -Werror
        )
  elseif(MSVC)
    target_compile_options(${TARGET} PRIVATE
            /W4     # Максимальный уровень предупреждений
            /WX     # Обрабатывать предупреждения как ошибки
            /permissive-  # Строгое соответствие стандартам
        )
  endif()

  set_target_properties(${TARGET} PROPERTIES
        COMPILE_WARNING_AS_ERROR ON
    )
endfunction()
