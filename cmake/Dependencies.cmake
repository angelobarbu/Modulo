# Dependencies.cmake — single home for every third-party dependency.
#
# `modulo_find_dependencies()` resolves, in one place:
#   - Homebrew binary libs: Qt 6.8+, libpqxx, libsodium
#   - CPM-pinned source libs: Catch2 v3, nlohmann-json
#
# A macro (not a function) so find_package results land in the caller's
# directory scope. Called exactly once, from the root CMakeLists.txt.

include_guard(GLOBAL)

# Source dependencies are cached outside the build tree so wiping build/
# does not re-download them (.cache/ is gitignored). Must be set BEFORE
# include(CPM): CPM initializes this cache variable itself on include, and
# a later set(... CACHE ...) would not override the existing entry.
set(CPM_SOURCE_CACHE
    "${CMAKE_SOURCE_DIR}/.cache/cpm"
    CACHE PATH "Download cache for CPM source dependencies")

include(CPM)

macro(modulo_find_dependencies)
    # --- Homebrew binary libraries -------------------------------------------
    # Qt path comes from CMAKE_PREFIX_PATH (set by the presets: /opt/homebrew/opt/qt).
    find_package(
        Qt6 6.8 REQUIRED
        COMPONENTS Core
                   Network
                   HttpServer
                   Qml
                   Quick
                   QuickControls2
                   Test
                   QuickTest)

    # libpqxx ships CMake package config (target: libpqxx::pqxx).
    find_package(libpqxx REQUIRED)

    # libsodium ships no CMake config, only pkg-config; locate it directly so
    # the build has no pkg-config dependency (works identically in Docker later).
    if(NOT TARGET sodium::sodium)
        find_path(MODULO_SODIUM_INCLUDE_DIR sodium.h)
        find_library(MODULO_SODIUM_LIBRARY sodium)
        if(NOT MODULO_SODIUM_INCLUDE_DIR OR NOT MODULO_SODIUM_LIBRARY)
            message(FATAL_ERROR "libsodium not found (brew install libsodium)")
        endif()
        add_library(sodium::sodium UNKNOWN IMPORTED)
        set_target_properties(
            sodium::sodium
            PROPERTIES IMPORTED_LOCATION "${MODULO_SODIUM_LIBRARY}"
                       INTERFACE_INCLUDE_DIRECTORIES "${MODULO_SODIUM_INCLUDE_DIR}")
    endif()

    # --- CPM source libraries (version-pinned) -------------------------------
    if(MODULO_BUILD_TESTS)
        cpmaddpackage("gh:catchorg/Catch2@3.8.1")
    endif()

    cpmaddpackage("gh:nlohmann/json@3.11.3")
endmacro()
