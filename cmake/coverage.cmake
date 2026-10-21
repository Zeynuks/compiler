option(ENABLE_COVERAGE "Собирать с отчетом о покрытии кода тестами" OFF)

# Функция custom_enable_coverage включает сбор покрытия кода для выбранной цели,
# если cmake с флагом -DENABLE_COVERAGE=ON.
function(custom_enable_coverage TARGET)
  if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(STATUS "Enabling coverage for target: ${TARGET}")
    target_compile_options(${TARGET} PRIVATE --coverage -O0 -g)
    target_link_options(${TARGET} PRIVATE --coverage)
  endif()
endfunction()
