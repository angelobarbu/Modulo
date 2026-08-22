# Dependencies.cmake — Third-party dependencies.
#
# `modulo_find_dependencies()` resolves every external dependency in one
# place. All of them are Homebrew binary libraries: Qt 6.8+, libpqxx, libsodium.
# There are no source-level dependencies (testing uses Qt Test).
#
# A macro so find_package results land in the caller's
# directory scope. Called from the root CMakeLists.txt.

include_guard(GLOBAL)

macro(modulo_find_dependencies)
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
endmacro()
