# Функция custom_enable_cpp23 включает стандарт C++23 для выбранной цели.
function(custom_enable_cpp23 TARGET)
  message(STATUS "Enabling C++23 for target: ${TARGET}")

  target_compile_features(${TARGET} PRIVATE cxx_std_23)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${TARGET} PRIVATE -std=c++23)
  elseif(MSVC)
    target_compile_options(${TARGET} PRIVATE /std:c++latest)
  endif()

  set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()
