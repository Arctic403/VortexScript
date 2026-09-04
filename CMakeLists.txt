cmake_minimum_required(VERSION 3.22)

project(VortexScript
    VERSION 0.1.0
    DESCRIPTION "Bounded command-language frontend for Vortex3D"
    LANGUAGES CXX
)

option(VORTEX_SCRIPT_BUILD_TESTS "Build VortexScript tests" ON)
option(VORTEX_SCRIPT_BUILD_CLI "Build VortexScript CLI" ON)
option(VORTEX_SCRIPT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
set(VORTEX_SCRIPT_EXPECT_POINTER_BITS "" CACHE STRING "Expected target pointer width for portability validation")

if(VORTEX_SCRIPT_EXPECT_POINTER_BITS)
    math(EXPR VORTEX_SCRIPT_POINTER_BITS "${CMAKE_SIZEOF_VOID_P} * 8")
    if(NOT VORTEX_SCRIPT_POINTER_BITS EQUAL VORTEX_SCRIPT_EXPECT_POINTER_BITS)
        message(FATAL_ERROR "VortexScript pointer width mismatch: expected ${VORTEX_SCRIPT_EXPECT_POINTER_BITS}, got ${VORTEX_SCRIPT_POINTER_BITS}")
    endif()
endif()

add_library(vortex_script_core STATIC src/compiler.cpp src/lexer.cpp src/parser.cpp)
add_library(VortexScript::core ALIAS vortex_script_core)
target_include_directories(vortex_script_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_include_directories(vortex_script_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(vortex_script_core PUBLIC cxx_std_20)
set_target_properties(vortex_script_core PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)

function(vortex_script_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(VORTEX_SCRIPT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(VORTEX_SCRIPT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
vortex_script_enable_warnings(vortex_script_core)

if(VORTEX_SCRIPT_BUILD_CLI AND NOT ANDROID)
    add_executable(vortex-script tools/main.cpp)
    target_link_libraries(vortex-script PRIVATE VortexScript::core)
    target_compile_features(vortex-script PRIVATE cxx_std_20)
    set_target_properties(vortex-script PROPERTIES CXX_EXTENSIONS OFF)
    vortex_script_enable_warnings(vortex-script)
endif()

if(ANDROID)
    # Link a real ELF shared object so CI verifies the static frontend can be consumed
    # by an Android native library and so its ELF page alignment can be inspected.
    add_library(vortex_script_android_smoke SHARED tests/android_link_smoke.cpp)
    target_link_libraries(vortex_script_android_smoke PRIVATE VortexScript::core)
    target_compile_features(vortex_script_android_smoke PRIVATE cxx_std_20)
    target_link_options(vortex_script_android_smoke PRIVATE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384")
    vortex_script_enable_warnings(vortex_script_android_smoke)
endif()

if(VORTEX_SCRIPT_BUILD_TESTS AND NOT ANDROID)
    include(CTest)
    enable_testing()
    add_executable(vortex_script_foundation_tests tests/foundation.cpp)
    target_link_libraries(vortex_script_foundation_tests PRIVATE VortexScript::core)
    target_compile_features(vortex_script_foundation_tests PRIVATE cxx_std_20)
    set_target_properties(vortex_script_foundation_tests PROPERTIES CXX_EXTENSIONS OFF)
    vortex_script_enable_warnings(vortex_script_foundation_tests)
    add_test(NAME vortex.script.foundation COMMAND vortex_script_foundation_tests)
endif()
