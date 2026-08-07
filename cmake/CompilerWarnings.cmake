# CompilerWarnings.cmake — project-wide warning configuration.
#
# Defines the `modulo_warnings` INTERFACE target carrying the warning flags
# shared by every first-party target, and `modulo_enable_warnings(<target>)`
# to attach them. Third-party code fetched via CPM is never touched.
#
# The flag set is controlled by the MODULO_WARNINGS_AS_ERRORS option
# (declared in the root CMakeLists.txt, enabled by the `dev` preset).

include_guard(GLOBAL)

add_library(modulo_warnings INTERFACE)

target_compile_options(
    modulo_warnings
    INTERFACE -Wall
              -Wextra
              -Wpedantic
              -Wconversion
              -Wshadow)

if(MODULO_WARNINGS_AS_ERRORS)
    target_compile_options(modulo_warnings INTERFACE -Werror)
endif()

# Attach the shared warning flags to a first-party target.
function(modulo_enable_warnings target)
    target_link_libraries(${target} PRIVATE modulo_warnings)
endfunction()
