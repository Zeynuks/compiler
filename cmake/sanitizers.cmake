# Функция custom_enable_sanitizers включает sanitizers для выбранной цели.
function(custom_enable_sanitizers TARGET)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      # Для Clang и GCC используем встроенные санитайзеры:
      # - address sanitizer
      # - undefined behavior sanitizer
      target_compile_options(${TARGET} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
      target_link_options(${TARGET} PRIVATE
                -fsanitize=address,undefined
            )
    elseif(MSVC)
      # MSVC имеет встроенные санитайзеры через /fsanitize (новые версии)
      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
        target_compile_options(${TARGET} PRIVATE /fsanitize=address)
      else()
        message(FATAL_ERROR "Sanitizers require MSVC 2019 16.9+")
      endif()
    endif()
  endif()
endfunction()
