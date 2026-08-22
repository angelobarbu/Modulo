# StaticAnalysis.cmake — clang-tidy integration.
#
# Defines `modulo_enable_clang_tidy(<target>)`, which honors the
# MODULO_CLANG_TIDY option (enabled by the `dev-tidy` preset). When on,
# every compile of the target also runs clang-tidy with the repo-root
# .clang-tidy configuration.
#
# Homebrew LLVM is keg-only, so the binary is referenced by absolute path;
# override with -DMODULO_CLANG_TIDY_EXE=... on other machines.

include_guard(GLOBAL)

set(MODULO_CLANG_TIDY_EXE
    "/opt/homebrew/opt/llvm/bin/clang-tidy"
    CACHE FILEPATH "clang-tidy executable used when MODULO_CLANG_TIDY is ON")

# Run clang-tidy alongside compilation for a first-party target.
function(modulo_enable_clang_tidy target)
    if(NOT MODULO_CLANG_TIDY)
        return()
    endif()

    if(NOT EXISTS "${MODULO_CLANG_TIDY_EXE}")
        message(FATAL_ERROR "MODULO_CLANG_TIDY is ON but clang-tidy was not found at "
                            "'${MODULO_CLANG_TIDY_EXE}' (brew install llvm, or set MODULO_CLANG_TIDY_EXE)")
    endif()

    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${MODULO_CLANG_TIDY_EXE}")
endfunction()
