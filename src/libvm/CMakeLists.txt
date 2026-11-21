add_library(libvm
        Instruction.h
        VirtualMachine.h
        VirtualMachine.cpp
)

target_include_directories(
        libvm
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

custom_enable_cpp23(libvm)
custom_enable_sanitizers(libvm)
custom_enable_strict_warnings(libvm)
custom_enable_clang_tidy(libvm)
custom_enable_coverage(libvm)
