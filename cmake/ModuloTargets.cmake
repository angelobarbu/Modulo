# ModuloTargets.cmake — declarative target creation for first-party code.
#
# Every CMakeLists.txt in the repo stays a short, generic call into one of
# these functions; all shared logic (C++23, include/src layout, warnings,
# sanitizers, clang-tidy, CTest registration) are included here.
#
#   modulo_add_library(<name> SOURCES ... [PUBLIC_DEPS ...] [PRIVATE_DEPS ...])
#       Static library following the module convention: public headers in
#       ./include (as <modulo/...>), implementation in ./src.
#
#   modulo_add_executable(<name> SOURCES ... [DEPS ...])
#       Application or tool binary.
#
#   modulo_add_test(<name> LABEL unit|integration SOURCES ... [DEPS ...])
#       Qt Test binary (one QObject test class, QTEST_GUILESS_MAIN), registered
#       with CTest under the given label (labels drive the `unit` /
#       `integration` / `all` test presets).
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
    # Single source of truth for the project version: the root project() call.
    target_compile_definitions(${target} PRIVATE MODULO_VERSION="${PROJECT_VERSION}")
    modulo_enable_warnings(${target})
    modulo_enable_sanitizers(${target})
    modulo_enable_clang_tidy(${target})
endfunction()

# Write a qt.conf beside an executable, pinning Qt's plugin/QML resolution to
# the Qt installation we actually link against. Without this, Homebrew's keg-only
# qt resolves plugins via the brew prefix root (/opt/homebrew/share/qt), which a
# different Qt formula (e.g. a newer qtbase) can shadow — the app then tries to
# load version-incompatible plugins and refuses to start.
function(_modulo_write_qt_conf target)
    if(NOT TARGET Qt6::Core)
        return()
    endif()

    # Executables land in the current binary dir; one qt.conf per directory
    # serves every binary in it (generating the same file twice is an error).
    get_property(_modulo_qt_conf_written DIRECTORY PROPERTY MODULO_QT_CONF_WRITTEN)
    if(_modulo_qt_conf_written)
        return()
    endif()
    set_property(DIRECTORY PROPERTY MODULO_QT_CONF_WRITTEN TRUE)

    get_filename_component(_modulo_qt_root "${Qt6_DIR}/../../.." ABSOLUTE)
    if(EXISTS "${_modulo_qt_root}/share/qt/plugins")
        set(_modulo_qt_prefix "${_modulo_qt_root}/share/qt") # Homebrew layout
    else()
        set(_modulo_qt_prefix "${_modulo_qt_root}") # official-installer layout
    endif()

    file(
        GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
        CONTENT "[Paths]\nPrefix = ${_modulo_qt_prefix}\n")
endfunction()

# Module convention: a target's tests live in ./tests and are picked up
# automatically when MODULO_BUILD_TESTS is ON — no per-module wiring needed.
function(_modulo_add_tests_subdirectory)
    if(MODULO_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
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
    _modulo_add_tests_subdirectory()
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
    _modulo_write_qt_conf(${name})
endfunction()

function(modulo_add_qml_app name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "URI" "SOURCES;QML_FILES;DEPS")

    if(NOT ARG_URI)
        message(FATAL_ERROR "modulo_add_qml_app(${name}): URI is required")
    endif()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "modulo_add_qml_app(${name}): SOURCES is required")
    endif()

    qt_add_executable(${name} ${ARG_SOURCES})
    qt_add_qml_module(
        ${name}
        URI ${ARG_URI}
        VERSION 1.0
        QML_FILES ${ARG_QML_FILES})

    # Apps follow the same include/src split as libraries, but their headers
    # are private — nobody links against an application.
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif()

    # qmltyperegistrar's generated registration file includes each QML-exposed
    # header by BASENAME only (guarded by __has_include, so a miss is silent
    # and surfaces as "undeclared identifier" instead). Make every listed
    # header's own directory an include dir so those basename includes resolve.
    foreach(source IN LISTS ARG_SOURCES)
        if(source MATCHES "\\.h$")
            get_filename_component(header_dir "${CMAKE_CURRENT_SOURCE_DIR}/${source}" DIRECTORY)
            target_include_directories(${name} PRIVATE "${header_dir}")
        endif()
    endforeach()

    if(ARG_DEPS)
        target_link_libraries(${name} PRIVATE ${ARG_DEPS})
    endif()

    _modulo_apply_common_settings(${name})
    _modulo_write_qt_conf(${name})
    _modulo_add_tests_subdirectory()
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

    # One Qt Test class per binary (QTEST_GUILESS_MAIN in the single source file).
    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test)
    if(ARG_DEPS)
        target_link_libraries(${name} PRIVATE ${ARG_DEPS})
    endif()

    # Shared test-support headers (<modulo/testing/...>): integration fixtures etc.
    target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/tests/support/include")

    _modulo_apply_common_settings(${name})
    _modulo_write_qt_conf(${name})

    add_test(NAME ${name} COMMAND ${name})
    # QSKIP() prints "SKIP   : ..." and exits 0; make CTest report the binary as
    # skipped (e.g. integration tests without a database), not passed.
    set_tests_properties(${name} PROPERTIES LABELS ${ARG_LABEL} SKIP_REGULAR_EXPRESSION "SKIP   : ")
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
    _modulo_write_qt_conf(${name})

    add_test(NAME ${name} COMMAND ${name})
    # QML tests need a QPA platform but no display: run them offscreen.
    set_tests_properties(${name} PROPERTIES LABELS ui ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()
