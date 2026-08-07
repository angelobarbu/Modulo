# ModuloTargets.cmake — declarative target creation for first-party code.
#
# Every CMakeLists.txt in the repo stays a short, generic call into one of
# these functions; all shared logic (C++23, include/src layout, warnings,
# sanitizers, clang-tidy, CTest registration) lives here.
#
#   modulo_add_library(<name> SOURCES ... [PUBLIC_DEPS ...] [PRIVATE_DEPS ...])
#       Static library following the module convention: public headers in
#       ./include (as <modulo/...>), implementation in ./src.
#
#   modulo_add_executable(<name> SOURCES ... [DEPS ...])
#       Application or tool binary.
#
#   modulo_add_test(<name> LABEL unit|integration SOURCES ... [DEPS ...])
#       Catch2 test binary, registered with CTest under the given label
#       (labels drive the `unit` / `integration` / `all` test presets).
#
#   modulo_add_qml_test(<name> QML_DIR <dir> SOURCES ... [DEPS ...])
#       Qt Quick Test binary running the tst_*.qml files in QML_DIR,
#       registered with CTest under the `ui` label.
#
# Test functions are no-ops when MODULO_BUILD_TESTS is OFF.

include_guard(GLOBAL)

include(CompilerWarnings)
include(Sanitizers)
include(StaticAnalysis)

# Settings common to every first-party target (never applied to third-party code).
function(_modulo_apply_common_settings target)
    target_compile_features(${target} PUBLIC cxx_std_23)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    modulo_enable_warnings(${target})
    modulo_enable_sanitizers(${target})
    modulo_enable_clang_tidy(${target})
endfunction()

function(modulo_add_library name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;PUBLIC_DEPS;PRIVATE_DEPS")

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "modulo_add_library(${name}): SOURCES is required")
    endif()

    add_library(${name} STATIC ${ARG_SOURCES})

    # Module convention: consumers include <modulo/...> from ./include;
    # implementation files may include internals from ./src.
    target_include_directories(
        ${name}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    if(ARG_PUBLIC_DEPS)
        target_link_libraries(${name} PUBLIC ${ARG_PUBLIC_DEPS})
    endif()
    if(ARG_PRIVATE_DEPS)
        target_link_libraries(${name} PRIVATE ${ARG_PRIVATE_DEPS})
    endif()

    _modulo_apply_common_settings(${name})
endfunction()

function(modulo_add_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;DEPS")

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "modulo_add_executable(${name}): SOURCES is required")
    endif()

    add_executable(${name} ${ARG_SOURCES})

    if(ARG_DEPS)
        target_link_libraries(${name} PRIVATE ${ARG_DEPS})
    endif()

    _modulo_apply_common_settings(${name})
endfunction()

function(modulo_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "LABEL" "SOURCES;DEPS")

    if(NOT MODULO_BUILD_TESTS)
        return()
    endif()

    if(NOT ARG_LABEL MATCHES "^(unit|integration)$")
        message(FATAL_ERROR "modulo_add_test(${name}): LABEL must be 'unit' or 'integration'")
    endif()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "modulo_add_test(${name}): SOURCES is required")
    endif()

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain)
    if(ARG_DEPS)
        target_link_libraries(${name} PRIVATE ${ARG_DEPS})
    endif()

    _modulo_apply_common_settings(${name})

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES LABELS ${ARG_LABEL})
endfunction()

function(modulo_add_qml_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "QML_DIR" "SOURCES;DEPS")

    if(NOT MODULO_BUILD_TESTS)
        return()
    endif()

    if(NOT ARG_QML_DIR)
        message(FATAL_ERROR "modulo_add_qml_test(${name}): QML_DIR is required")
    endif()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "modulo_add_qml_test(${name}): SOURCES is required")
    endif()

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::QuickTest Qt6::Qml)
    if(ARG_DEPS)
        target_link_libraries(${name} PRIVATE ${ARG_DEPS})
    endif()

    # QUICK_TEST_SOURCE_DIR points the QUICK_TEST_MAIN runner at the tst_*.qml files.
    target_compile_definitions(${name} PRIVATE QUICK_TEST_SOURCE_DIR="${ARG_QML_DIR}")

    _modulo_apply_common_settings(${name})

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES LABELS ui)
endfunction()
