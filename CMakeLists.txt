# Modulo — personal investment tracker.
#
# This file stays deliberately thin: options, toolkit includes, dependency
# resolution, and subdirectory wiring. All build logic lives as modulo_*
# functions in cmake/ (see cmake/ModuloTargets.cmake).

cmake_minimum_required(VERSION 3.28)

project(
    Modulo
    VERSION 0.1.0
    DESCRIPTION "Personal investment tracker"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# --- Options (set via CMakePresets.json) -------------------------------------
option(MODULO_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(MODULO_BUILD_TESTS "Build the test suites" ON)
option(MODULO_CLANG_TIDY "Run clang-tidy during compilation" OFF)
set(MODULO_SANITIZERS
    ""
    CACHE STRING "Comma-separated -fsanitize= list, e.g. 'address,undefined'")

# --- Toolkit & dependencies --------------------------------------------------
include(ModuloTargets)
include(Dependencies)
modulo_find_dependencies()

qt_standard_project_setup(REQUIRES 6.8)

enable_testing()

# --- Project targets ---------------------------------------------------------
# Subdirectories are appended as the increments introduce them:
# add_subdirectory(libs/core)      # Increment 1, Step 5
# add_subdirectory(libs/api)       # Increment 1, Step 5
add_subdirectory(server)
# add_subdirectory(client)         # Increment 1, Step 5
